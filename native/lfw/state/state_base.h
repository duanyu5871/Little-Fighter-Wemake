#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

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
  virtual Value bounced() const { return Value(); }
  virtual void set_bounced(const Value& v) { (void)v; }
  virtual bool has_catcher() const { return false; }
  virtual void catcher_drop_catching() {}
  virtual void set_facing(const Value& v) { (void)v; }
  virtual Value world_dataset(const std::u16string& key) const {
    (void)key;
    return Value();
  }
  virtual Value data_indexes_bouncing() const { return Value(); }
  virtual Value data_indexes_lying() const { return Value(); }
  virtual Value world_entities() const { return Value(); }
  virtual bool is_fighter_ref(const Value& o) const {
    (void)o;
    return false;
  }
  virtual bool is_self_ref(const Value& o) const {
    (void)o;
    return false;
  }
  virtual bool is_ally_ref(const Value& o) const {
    (void)o;
    return false;
  }
  virtual Value ref_hp(const Value& o) const {
    (void)o;
    return Value();
  }
  virtual double ref_position_x(const Value& o) const {
    (void)o;
    return 0;
  }
  virtual double ref_position_z(const Value& o) const {
    (void)o;
    return 0;
  }
  virtual Value ground_segment(double x, double z) {
    (void)x;
    (void)z;
    return Value();
  }
  virtual double ground_y(const Value& segment, double x, double z) {
    (void)segment;
    (void)x;
    (void)z;
    return 0;
  }
  virtual bool is_on_ground() const { return false; }
  virtual Value ground_y() const { return Value(); }
  virtual Value get_sudden_death_frame() { return Value(); }
  virtual void holding_set_team(const Value& v) { (void)v; }
  virtual Value frame_on_landing() const { return Value(); }
  virtual Value frame_info() const { return Value(); }
  virtual double ctrl_ud() const { return 0; }
  virtual double ctrl_lr() const { return 0; }
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
  virtual void transfrom_to_another() {}
  virtual Value find_auto_frame() { return Value(); }
  virtual void transform(const Value& data) { (void)data; }
  virtual Value datas_find(const std::u16string& oid) {
    (void)oid;
    return Value();
  }
  virtual Value datas_find_fighter(const std::u16string& oid) {
    (void)oid;
    return Value();
  }
  virtual void set_shaking(const Value& v) { (void)v; }
  virtual void set_motionless(const Value& v) { (void)v; }
  virtual void world_callbacks_call(const std::u16string& name) { (void)name; }
  virtual void handle_ground_velocity_decay() {}
  virtual Value data_indexes_default() const { return Value(); }
  virtual Value data_indexes_landing_1() const { return Value(); }
  virtual Value data_indexes_landing_2() const { return Value(); }
  virtual Value data_indexes_heavy_obj_walk() const { return Value(); }
  virtual bool has_data_indexes() const { return false; }
  virtual Value data_indexes_on_ground() const { return Value(); }
  virtual Value data_indexes_throwings() const { return Value(); }
  virtual Value data_base() const { return Value(); }
  virtual Value base_type() const { return Value(); }
  virtual Value drop_hurted() const { return Value(); }
  virtual void set_drop_hurted(const Value& v) { (void)v; }
  virtual void leave_ground() {}
  virtual Value frame_id() const { return Value(); }
  virtual Value find_align_frame(const Value& fid, const Value& throwings,
                                 const Value& in_the_skys) {
    (void)fid;
    (void)throwings;
    (void)in_the_skys;
    return Value();
  }
  virtual Value data_frames() const { return Value(); }
  virtual void play_sound(const Value& sounds) { (void)sounds; }
  virtual void apply_opoints(const std::vector<Value>& opoints) { (void)opoints; }
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
