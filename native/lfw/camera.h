#pragma once

#include "lfw/core/value.h"
#include "lfw/defines/i_vector2.h"

namespace lfw {

class World;

// TS `Camera.update()` 直接读 `this.world.stage` / `this.world.bg` / `this.world.dataset`。
// 切片 4CK-1 之前这三条走的是 `ICameraWorld` 缝（TS 的 `World` 那时还没移植），现在
// 相机直接持真 `World*`，读的就是 `World::world_stage()` / `world_bg()` / `world_dataset()`
// —— 它们给 `Value` 而不是拆成 9 个数字：TS 读的是**属性**，`undefined` / 字符串 / 缺字段
// 都得按 JS 的强转（`to_number`）与 `??` 走；台面也就能观察到「`world.stage` 被读了几次」。
//
// 注意 `world.stage` 在 `update()` 里被读**两次**（顶层解构一次、y 块里 `this.world.stage.far`
// 一次），端口照抄。
//
// TS `Camera`。
class Camera {
 public:
  explicit Camera(World* world);

  World* world() const { return _world; }
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
  World* _world = nullptr;
  bool _has_locked = false;
  bool _has_dested = false;
  Vector2 _locked;
  Vector2 _dested;
};

}
