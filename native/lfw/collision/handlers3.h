#pragma once

#include <functional>
#include <string>

#include "lfw/buff/buff.h"
#include "lfw/collision/collision.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IH3Entity {
  virtual ~IH3Entity() = default;
  virtual const std::u16string& id() const = 0;
  virtual void velocity(double& x, double& y, double& z) const = 0;
  virtual void set_velocity(const Value& x, const Value& y, const Value& z) = 0;
  virtual void position(double& x, double& y, double& z) const = 0;
  virtual Value team() const = 0;
  virtual void set_team(const Value& v) = 0;
  virtual bool has_bearer() const = 0;
  virtual Value base_type() const = 0;
  virtual Value state() const = 0;
  virtual Value data_in_the_skys_first() const = 0;
  virtual Value data_base_hit_sounds() const = 0;
  virtual Value hp() const = 0;
  virtual void set_hp(const Value& v) = 0;
  virtual Value hp_r() const = 0;
  virtual void set_hp_r(const Value& v) = 0;
  virtual Value armor() const = 0;
  virtual Value toughness() const = 0;
  virtual void set_toughness(const Value& v) = 0;
  virtual Value toughness_max() const = 0;
  virtual Value itr_fall(const Value& itr) const = 0;
  virtual Value dataset(const std::u16string& key) const = 0;
  virtual void set_motionless(const Value& v) = 0;
  virtual void set_shaking(const Value& v) = 0;
  virtual void enter_frame_by_id(const Value& id) = 0;
  virtual void play_sound(const Value& sounds) = 0;
  virtual buff::IBuffEntity* buff_entity() = 0;
};

struct Handlers3Env {
  std::function<IH3Entity*(const std::u16string& id)> find_entity;
  std::function<bool(const IH3Entity& e)> is_ball;
  std::function<bool(const IH3Entity& e)> is_weapon;
  std::function<void(const Cube& a, const Cube& b, double& x, double& y, double& z)> spark_point;
  std::function<void(double x, double y, double z, const Value& type)> spark;
  std::function<void(const Value& sounds, double x, double y, double z)> play_sound_global;
  std::function<bool(Collision& c)> is_armor_work;
};

const Handlers3Env& handlers3_env();
void set_handlers3_env(const Handlers3Env& env);

void handle_itr_kind_whirlwind(Collision& c);
void handle_ball_is_hit_a(Collision& c);
void handle_ball_is_hit_b(Collision& c);
bool handle_armor(Collision& c);

}
}
