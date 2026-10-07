#pragma once

#include <functional>
#include <string>

#include "lfw/collision/handlers2.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

// `IHandlerEntity` 用**虚基类**：`IWeaponIsHitEntity` 也从它派生 ⇒ 两个窄接口可以落在同一个
// 实现类上（见 `entity/entity_collision_view.h` 的 `EntityCollisionView`）。
struct IFallEntity : virtual IHandlerEntity {
  virtual Value facing() const = 0;
  virtual double velocity_x() const = 0;
  virtual void spark_point(const Cube& a, const Cube& b, double& x, double& y, double& z) = 0;
  virtual Value data_indexes_fire() const = 0;
  virtual Value data_indexes_critical_hit() const = 0;
  virtual Value holding_base_type() const = 0;
  virtual void drop_holding() = 0;
};

struct FallEnv {
  std::function<IFallEntity*(const std::u16string& id)> find_entity;
  std::function<bool(const IFallEntity& e)> is_fighter;
  std::function<void(const Value& x, const Value& y, const Value& z, const Value& type)> spark;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_fall(Collision& c);

const FallEnv& fall_env();
void set_fall_env(const FallEnv& env);

}
}
