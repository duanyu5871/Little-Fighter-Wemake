#pragma once

#include <functional>
#include <string>

#include "lfw/collision/handlers2.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IWeaponIsHitEntity : IHandlerEntity {
  virtual bool has_bearer() const = 0;
  virtual void set_dropping(bool v) = 0;
  virtual Value base_type() const = 0;
  virtual Value facing() const = 0;
  virtual Value team() const = 0;
  virtual void set_team(const Value& v) = 0;
  virtual Value data_id() const = 0;
  virtual Value data_indexes_throwings() const = 0;
  virtual Value data_indexes_in_the_skys() const = 0;
  virtual void leave_ground() = 0;
};

struct SparkPoint {
  Value x;
  Value y;
  Value z;
};

struct WeaponIsHitEnv {
  std::function<IWeaponIsHitEntity*(const std::u16string& id)> find_entity;
  std::function<SparkPoint(const Cube& a, const Cube& b)> spark_point;
  std::function<void(const Value& x, const Value& y, const Value& z,
                     const std::u16string& kind)>
      spark;
  std::function<void(const std::u16string& mark)> mt_mark;
  std::function<Value(const Value& indexes)> mt_pick;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_weapon_is_hit(Collision& c);

const WeaponIsHitEnv& weapon_is_hit_env();
void set_weapon_is_hit_env(const WeaponIsHitEnv& env);

}
}
