#include "lfw/camera.h"

#include <string>
#include <variant>

#include "lfw/defines/defines_data.h"
#include "lfw/ditto/instance.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

namespace {

// `Defines.MODERN_SCREEN_HEIGHT`（`defines.ts` 里是 450）。走运行时表 ⇒ 与 TS 读的是同一个值。
double modern_screen_height() { return defines::num(u"Defines.MODERN_SCREEN_HEIGHT"); }

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

Camera::Camera(ICameraWorld* world) : _world(world) {
  destination = ditto::vec2();
  position = ditto::vec2();
  velocity = ditto::vec2();
}

void Camera::reset() {
  jump_x(0);
  jump_y(0);
  _has_locked = false;
  _has_dested = false;
}

void Camera::jump_x(double x) {
  velocity.x = 0;
  position.x = destination.x = x;
}

void Camera::jump_y(double y) {
  velocity.y = 0;
  position.y = destination.y = y;
}

void Camera::undest() { _has_dested = false; }

void Camera::dest(double x, double y) { _dested = ditto::vec2(x, y); _has_dested = true; }

void Camera::unlock() { _has_locked = false; }

void Camera::lock(double x, double y) { _locked = ditto::vec2(x, y); _has_locked = true; }

void Camera::update() {
  if (_has_locked) {
    jump_x(_locked.x);
    jump_y(_locked.y);
    return;
  }

  // TS 顶层解构：`const { stage, bg, dataset: { atom_time, screen_w, screen_h } } = this.world;`
  const Value stage = _world->world_stage();
  const Value bg = _world->world_bg();
  const Value dataset = _world->world_dataset();
  const double atom_time = to_number(field_or(dataset, u"atom_time"));
  const double screen_w = to_number(field_or(dataset, u"screen_w"));
  const double screen_h = to_number(field_or(dataset, u"screen_h"));

  do {
    // TS：`const { cam_l, cam_r } = stage;`
    const double min_cam_x = to_number(field_or(stage, u"cam_l"));
    const double max_cam_x =
        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_x"));
    const double max_vx_ratio = 50;
    const double acc_x_ratio = 1;
    // `this._dested?.x ?? this.destination.x`：TS 的 `??` 也吞 `_dested.x` 是 nullish 的情况，
    // 端口的分量是 double ⇒ 只区分「有没有 `_dested`」。
    destination.x = clamp(_has_dested ? _dested.x : destination.x, min_cam_x, max_cam_x);
    if (position.x < min_cam_x || position.x > max_cam_x) {
      velocity.x = 0;
      position.x = clamp(position.x, min_cam_x, max_cam_x);
      break;
    }
    if (round(position.x) == round(destination.x)) break;

    const double acc_x = min(atom_time * acc_x_ratio,
                             atom_time * 0.7 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_w);
    const double direction_x = position.x > destination.x ? -1 : 1;
    const double max_vx = direction_x * max_vx_ratio * acc_x;
    if (sign(velocity.x) != direction_x) velocity.x = 0;
    if (abs(velocity.x) < abs(max_vx))
      velocity.x += acc_x * direction_x;
    else
      velocity.x = max_vx;
    if (direction_x < 0)
      position.x = max(destination.x, position.x + velocity.x);
    else
      position.x = min(destination.x, position.x + velocity.x);
  } while (false);

  do {
    const double height = to_number(field_or(bg, u"height"));
    if (height <= modern_screen_height()) {
      position.y = destination.y = 0;
      break;
    }
    // TS：`const { far } = this.world.stage;` ⇒ `world.stage` 第二次读。
    const double far = to_number(field_or(_world->world_stage(), u"far"));
    const double max_vy_ratio = 50;
    const double acc_y_ratio = 1;
    const double cam_y = _has_dested ? _dested.y : destination.y;
    // `bg.zoom_y ?? 1`：只有 `null` / `undefined` 落回 1（`0` 保留）。
    const Value zoom_y_v = field_or(bg, u"zoom_y");
    const double zoom_y = is_nullish(zoom_y_v) ? 1 : to_number(zoom_y_v);
    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() / zoom_y);
    destination.y = clamp(cam_y, 0, cam_max_y);
    const double acc_y = min(atom_time * acc_y_ratio,
                             atom_time * 0.7 * (acc_y_ratio * abs(position.y - destination.y)) /
                                 screen_h);
    if (round(position.y) == round(destination.y)) break;
    const double direction_y = position.y > destination.y ? -1 : 1;
    const double max_vy = direction_y * max_vy_ratio * acc_y;
    if (sign(velocity.y) != direction_y) velocity.y = 0;
    if (abs(velocity.y) < abs(max_vy))
      velocity.y += acc_y * direction_y;
    else
      velocity.y = max_vy;
    if (direction_y < 0)
      position.y = max(destination.y, position.y + velocity.y);
    else
      position.y = min(destination.y, position.y + velocity.y);
  } while (false);
}

}
