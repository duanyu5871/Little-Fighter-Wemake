#pragma once

#include "lfw/core/value.h"
#include "lfw/defines/i_vector2.h"

namespace lfw {

// `Camera.update()` 从宿主世界读的那三个对象（TS 里是 `World` 的 `stage` / `bg` / `dataset`）。
// 给 `Value` 而不是拆成 9 个数字：TS 读的是**属性**，`undefined` / 字符串 / 缺字段都得按 JS 的
// 强转（`to_number`）与 `??` 走；台面也就能观察到「`world.stage` 被读了几次」。
//
// 注意 `world.stage` 在 `update()` 里被读**两次**（顶层解构一次、y 块里 `this.world.stage.far`
// 一次），端口照抄。
class ICameraWorld {
 public:
  virtual ~ICameraWorld() = default;
  virtual Value world_stage() = 0;
  virtual Value world_bg() = 0;
  virtual Value world_dataset() = 0;
};

// TS `Camera`。
class Camera {
 public:
  explicit Camera(ICameraWorld* world);

  ICameraWorld* world() const { return _world; }
  // TS `get locked()` / `get dested()` 是 `IVector2 | null` ⇒ 端口给指针，没锁 / 没目标就是 `nullptr`。
  const Vector2* locked() const { return _has_locked ? &_locked : nullptr; }
  const Vector2* dested() const { return _has_dested ? &_dested : nullptr; }

  Vector2 destination;
  Vector2 position;
  Vector2 velocity;

  void reset();
  void jump_x(double x);
  void jump_y(double y);
  void undest();
  void dest(double x, double y);
  void unlock();
  void lock(double x, double y);
  void update();

 private:
  ICameraWorld* _world = nullptr;
  bool _has_locked = false;
  bool _has_dested = false;
  Vector2 _locked;
  Vector2 _dested;
};

}
