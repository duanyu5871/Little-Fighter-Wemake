#pragma once

#include <functional>
#include <string>

#include "lfw/collision/collision.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IH4Entity {
  virtual ~IH4Entity() = default;
  virtual const std::u16string& id() const = 0;
  virtual Value data() const = 0;
  virtual Value hp() const = 0;
  virtual void set_hp(const Value& v) = 0;
  virtual Value hp_r() const = 0;
  virtual void set_hp_r(const Value& v) = 0;
  virtual Value state() const = 0;
  virtual Value facing() const = 0;
  virtual Value base_type() const = 0;
  virtual void velocity(double& x, double& y, double& z) const = 0;
  virtual void set_velocity(const Value& x, const Value& y, const Value& z) = 0;
  virtual Value frame_id() const = 0;
  virtual Value data_indexes_throwings() const = 0;
  virtual Value data_indexes_in_the_skys() const = 0;
  virtual Value arest() const = 0;
  virtual void set_arest(const Value& v) = 0;
  virtual void enter_frame(const Value& info) = 0;
  virtual void set_dropping(bool v) = 0;
  virtual Value data_base_hit_sounds() const = 0;
  virtual void play_sound(const Value& sounds) = 0;
};

struct Handlers4Env {
  std::function<IH4Entity*(const std::u16string& id)> find_entity;
  std::function<bool(const IH4Entity& e)> is_fighter;
  std::function<Value(const Value& frame_id, const Value& throwings, const Value& in_the_skys)>
      find_align_frame;
};

const Handlers4Env& handlers4_env();
void set_handlers4_env(const Handlers4Env& env);

void handle_ball_hit_other(Collision& c);
void handle_weapon_hit_other(Collision& c);

}
}
