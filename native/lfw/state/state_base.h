#pragma once

#include <functional>
#include <string>
#include <utility>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"

namespace lfw {
namespace state {

class IStateEntity : public buff::IBuffEntity {
 public:
  virtual Value velocity_x() const = 0;
  virtual Value velocity_z() const = 0;
  virtual Value facing() const { return Value(); }
  virtual Value holding_base_type() const { return Value(); }
  virtual bool has_holding() const { return false; }
  virtual Value fall_value() const { return Value(); }
  virtual Value fall_value_max() const { return Value(); }
  virtual void set_fall_value(const Value& v) { (void)v; }
  virtual bool is_on_ground() const { return false; }
  virtual Value ground_y() const { return Value(); }
  virtual Value get_sudden_death_frame() { return Value(); }
  virtual void holding_set_team(const Value& v) { (void)v; }
  virtual Value frame_on_landing() const { return Value(); }
  virtual Value frame_info() const { return Value(); }
  virtual bool ctrl_ud() const { return false; }
  virtual bool ctrl_lr() const { return false; }
  virtual bool holding_is_weapon() const { return false; }
  virtual double handle_wait_flag(const Value& wait, const Value& frame) {
    (void)wait;
    (void)frame;
    return 0;
  }
  virtual void enter_frame(const Value& frame) { (void)frame; }
  virtual void enter_frame_by_id_fallback(const std::u16string& id, bool fallback) {
    (void)fallback;
    enter_frame_by_id(id);
  }
  virtual void drop_holding() {}
  virtual void handle_ground_velocity_decay() {}
  virtual Value data_indexes_default() const { return Value(); }
  virtual Value data_indexes_landing_1() const { return Value(); }
  virtual Value data_indexes_landing_2() const { return Value(); }
  virtual Value data_indexes_heavy_obj_walk() const { return Value(); }
  virtual Value data_frames() const { return Value(); }
};

struct StateEnv {
  const buff::BuffEnv* buff_env = nullptr;
};

const StateEnv& state_env();
void set_state_env(const StateEnv& env);

class State_Base {
 public:
  explicit State_Base(Value state) : _state(std::move(state)) {}
  virtual ~State_Base() = default;

  const Value& state() const { return _state; }

  std::function<void(IStateEntity& e)> pre_update;
  std::function<void(IStateEntity& e, const Value& prev_frame)> enter;
  std::function<void(IStateEntity& e)> on_dead;
  std::function<void(IStateEntity& e, const Value& velocity)> on_landing;
  std::function<Value(IStateEntity& e)> get_gravity;
  std::function<Value(IStateEntity& e)> get_sudden_death_frame;
  std::function<Value(IStateEntity& e)> get_caught_end_frame;
  std::function<Value(IStateEntity& e)> get_auto_frame;
  std::function<Value(IStateEntity& e, const Value& id)> find_frame_by_id;
  std::function<void(IStateEntity& e)> on_leave_ground;

  virtual void update(IStateEntity& e);
  virtual void leave(IStateEntity& e, const Value& next_frame);
  virtual void on_restrict(IStateEntity& e, double x, double y, double z);

 protected:
  Value _state;
};

}
}
