#pragma once

#include <functional>
#include <string>

#include "lfw/buff/buff.h"
#include "lfw/collision/calc_itr_velocity.h"
#include "lfw/collision/handlers.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IHandlerEntity {
  virtual ~IHandlerEntity() = default;
  virtual const std::u16string& id() const = 0;
  virtual Value hp() const = 0;
  virtual void set_hp(const Value& v) = 0;
  virtual Value hp_r() const = 0;
  virtual void set_hp_r(const Value& v) = 0;
  virtual void set_toughness(const Value& v) = 0;
  virtual Value data() const = 0;
  virtual Value data_indexes_ice() const = 0;
  virtual Value data_base_hit_sounds() const = 0;
  virtual Value dataset(const std::u16string& key) const = 0;
  virtual bool marks_has(const std::u16string& kind) const = 0;
  virtual bool catching() const = 0;
  virtual void set_catching(IHandlerEntity* v) = 0;
  virtual Value catch_time_max() const = 0;
  virtual void set_catch_time(const Value& v) = 0;
  virtual IHandlerEntity* catcher() const = 0;
  virtual void set_catcher(IHandlerEntity* v) = 0;
  virtual Value resting() const = 0;
  virtual void set_resting(const Value& v) = 0;
  virtual Value fall_value() const = 0;
  virtual void set_fall_value(const Value& v) = 0;
  virtual Value fall_value_max() const = 0;
  virtual Value defend_value() const = 0;
  virtual void set_defend_value(const Value& v) = 0;
  virtual Value defend_value_max() const = 0;
  virtual Value itr_fall(const Value& itr) const = 0;
  virtual Value src_emitter() const = 0;
  virtual Value shaking() const = 0;
  virtual void set_shaking(const Value& v) = 0;
  virtual Value motionless() const = 0;
  virtual void set_velocity(const Value& x, const Value& y, const Value& z) = 0;
  virtual void enter_frame(const Value& info) = 0;
  virtual void enter_frame_by_id(const Value& id) = 0;
  virtual void play_sound(const Value& sounds) = 0;
  virtual buff::IBuffEntity* buff_entity() = 0;
};

struct Handlers2Env {
  std::function<void(const std::u16string& msg)> warn;
  std::function<Value()> hp_recoverability;
  std::function<IHandlerEntity*(const std::u16string& id)> find_entity;
  std::function<void(IHandlerEntity* a, const Value& injury, IHandlerEntity* v,
                     const Value& prev_hp)>
      summary_apply_damage;
  std::function<const buff::BuffEnv*()> buff_env;
  std::function<bool(const IHandlerEntity& e)> is_fighter;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_injury(Collision& c, double scale, bool keep_toughness);
void handle_itr_catch(Collision& c);
void handle_itr_kind_freeze(Collision& c);
void handle_itr_effect_freeze(Collision& c);
void handle_john_shield_hit_other_ball(Collision& c);

const Handlers2Env& handlers2_env();
void set_handlers2_env(const Handlers2Env& env);

}
}
