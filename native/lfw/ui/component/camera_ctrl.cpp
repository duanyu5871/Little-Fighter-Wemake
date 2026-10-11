#include "lfw/ui/component/camera_ctrl.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "lfw/bg/background.h"
#include "lfw/camera.h"
#include "lfw/controller/key_status.h"
#include "lfw/core/js_string.h"
#include "lfw/defines/cmd.h"
#include "lfw/entity/entity.h"
#include "lfw/helper/characters_helper.h"
#include "lfw/keys.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

namespace {

// JS `arr.at(i)`：负数从尾部数，越界 ⇒ undefined。
Entity* entity_at(const std::vector<Entity*>& v, double idx) {
  const double len = static_cast<double>(v.size());
  if (len <= 0 || std::isnan(idx)) return nullptr;
  double i = idx;
  if (i < 0) i += len;
  if (i < 0 || i >= len) return nullptr;
  return v[static_cast<size_t>(i)];
}

}

const ClazzTag* CameraCtrl::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& CameraCtrl::TAGS() {
  static const std::vector<std::u16string> tags{u"CameraCtrl"};
  return tags;
}

void CameraCtrl::set_staring(Entity* v) {
  if (v == _staring) return;
  if (_staring != nullptr) {
    _staring->set_outline_width(1);
    _staring->set_outline_color(u"");
  }
  _staring = v;
}

void CameraCtrl::update(double dt) {
  Keys& ks = keys();
  const double now = ks.time();
  controller::KeyStatus* const kj = ks.get(u"j");
  controller::KeyStatus* const kd = ks.get(u"d");
  controller::KeyStatus* const kL = ks.get(u"L");
  controller::KeyStatus* const kR = ks.get(u"R");
  controller::KeyStatus* const kU = ks.get(u"U");
  controller::KeyStatus* const kD = ks.get(u"D");
  // `this.LR` / `this.UD`：按住态（`is_end` 取反），UD = U - D。
  const double lr = (kR->is_end() ? 0.0 : 1.0) - (kL->is_end() ? 0.0 : 1.0);
  const double ud = (kU->is_end() ? 0.0 : 1.0) - (kD->is_end() ? 0.0 : 1.0);
  if (!kj->is_end()) {
    auto_ = true;
    if (kL->is_start(now)) focus_lr(-1);
    if (kR->is_start(now)) focus_lr(1);
    if (kU->is_start(now)) focus_ud(-1);
    if (kD->is_start(now)) focus_ud(1);
  } else if (!kd->is_end()) {
    lfw().push_cmd({std::u16string(cmd::kDIST_CAM)});
    set_staring(nullptr);
    auto_ = true;
  } else if (lr != 0 || ud != 0) {
    auto_ = false;
    // TS `let { camera: { position: { x, y } } } = this.world;` 是解构拷贝：只推坐标、不写回相机。
    double x = world().camera().position.x;
    double y = world().camera().position.y;
    x += 5 * dt * lr;
    y += 5 * dt * ud;
    lfw().push_cmd({std::u16string(cmd::kDIST_CAM),
                    number_to_string(x) + u"," + number_to_string(y)});
  }
  if (_staring != nullptr) {
    const double atom_time = to_number(field_or(world().world_dataset(), u"atom_time"));
    _staring->set_outline_width(round((std::cos(world().lifetime() * atom_time) + 1) * 2));
  }
  if (_staring != nullptr && (_staring->hp() <= 0 || _staring->mounted() == 0)) focus_lr(1);
  if (auto_ && _staring != nullptr) {
    const Value ds = world().world_dataset();
    const double screen_w = to_number(field_or(ds, u"screen_w"));
    const double screen_h = to_number(field_or(ds, u"screen_h"));
    const Background* const bg = world().bg();
    double zx = bg != nullptr ? bg->zoom_x() : 1.0;
    double zy = bg != nullptr ? bg->zoom_y() : 1.0;
    // TS 的 `zoom_x || 1`：0 / NaN 都回退 1。
    if (zx == 0.0 || std::isnan(zx)) zx = 1.0;
    if (zy == 0.0 || std::isnan(zy)) zy = 1.0;
    const Vector3& pos = _staring->position;
    const double cam_x = pos.x - screen_w / (2 * zx);
    const double cam_y = -0.5 * round(pos.z) - screen_h / (2 * zy);
    lfw().push_cmd({std::u16string(cmd::kDIST_CAM),
                    number_to_string(cam_x) + u"," + number_to_string(cam_y)});
  }
}

void CameraCtrl::on_stop() {
  lfw().push_cmd({std::u16string(cmd::kDIST_CAM)});
  if (_staring != nullptr) {
    _staring->set_outline_width(1);
    _staring->set_outline_color(u"");
  }
}

void CameraCtrl::focus_lr(double direction) {
  const std::vector<Entity*> src = candidates ? candidates() : lfw().characters_helper().all();
  std::vector<Entity*> fighters;
  for (Entity* const e : src) {
    if (e != nullptr && e->hp() > 0) fighters.push_back(e);
  }
  // TS `Array.sort` 是稳定排序（ES2019+）⇒ 端口用 stable_sort。
  std::stable_sort(fighters.begin(), fighters.end(),
                   [](Entity* a, Entity* b) { return a->position.x < b->position.x; });
  if (_staring == nullptr) {
    set_staring(entity_at(fighters, direction < 0 ? static_cast<double>(fighters.size()) - 1
                                                  : 0.0));
  } else {
    double idx = -1;
    for (size_t i = 0; i < fighters.size(); ++i) {
      if (fighters[i] == _staring) {
        idx = static_cast<double>(i);
        break;
      }
    }
    const double len = static_cast<double>(fighters.size());
    if (len > 0) set_staring(entity_at(fighters, std::fmod(idx + len + direction, len)));
  }
  auto_ = staring() != nullptr;
}

void CameraCtrl::focus_ud(double direction) {
  std::vector<Entity*> fighters = lfw().characters_helper().all();
  std::vector<Entity*> alive;
  for (Entity* const e : fighters) {
    if (e != nullptr && e->hp() > 0) alive.push_back(e);
  }
  fighters = std::move(alive);
  std::stable_sort(fighters.begin(), fighters.end(),
                   [](Entity* a, Entity* b) { return a->position.z < b->position.z; });
  if (_staring == nullptr) {
    set_staring(entity_at(fighters, direction < 0 ? static_cast<double>(fighters.size()) - 1
                                                  : 0.0));
  } else {
    double idx = -1;
    for (size_t i = 0; i < fighters.size(); ++i) {
      if (fighters[i] == _staring) {
        idx = static_cast<double>(i);
        break;
      }
    }
    const double len = static_cast<double>(fighters.size());
    if (len > 0) set_staring(entity_at(fighters, std::fmod(idx + len + direction, len)));
  }
  auto_ = staring() != nullptr;
}

}
}
