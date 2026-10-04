#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/entity/drink_info.h"

namespace lfw {
namespace state {

class IStateEntity : public buff::IBuffEntity {
 public:
  virtual Value velocity_x() const = 0;
  virtual Value velocity_z() const = 0;
  virtual Value hp_max() const { return Value(); }
  virtual Value facing() const { return Value(); }
  virtual Value holding_base_type() const { return Value(); }
  virtual bool has_holding() const { return false; }
  virtual DrinkInfo* holding_drink() const { return nullptr; }
  virtual void holding_set_hp(const Value& v) { (void)v; }
  virtual void holding_set_hp_r(const Value& v) { (void)v; }
  virtual void holding_set_velocity(const Value& x, const Value& y, const Value& z) {
    (void)x;
    (void)y;
    (void)z;
  }
  // `holding.lfw.mt.mark = …`（`CharacterState_Drink` 的掉落分支）。
  virtual void holding_mt_mark(const std::u16string& mark) { (void)mark; }
  virtual Value holding_mt_range(double lo, double hi) {
    (void)lo;
    (void)hi;
    return Value();
  }
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
  // `e.position.x = x` 这类**直写**：TS 的 `State_Base.on_restrict` 直接改 `Entity.position`
  // 的三个字段，不经过 `set_position`。宿主覆盖它以便跳过状态钩子（否则会递归），其余实现
  // 落到 `set_position`。
  virtual void assign_position(double x, double y, double z) { set_position(x, y, z); }
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
  virtual Value lfw_new_team() const { return Value(); }
  virtual Value frame_on_landing() const { return Value(); }
  virtual Value frame_behavior() const { return Value(); }
  virtual Value frame_info() const { return Value(); }
  virtual double ctrl_ud() const { return 0; }
  virtual double ctrl_lr() const { return 0; }
  virtual bool ctrl_is_bot() const { return false; }
  virtual bool ctrl_is_end(const std::u16string& key) const {
    (void)key;
    return true;
  }
  virtual Value jumping_x() const { return Value(); }
  virtual void set_jumping_x(const Value& v) { (void)v; }
  virtual Value jumping_y() const { return Value(); }
  virtual void set_jumping_y(const Value& v) { (void)v; }
  virtual Value jumping_z() const { return Value(); }
  virtual void set_jumping_z(const Value& v) { (void)v; }
  virtual Value jumping_t() const { return Value(); }
  virtual void set_jumping_t(const Value& v) { (void)v; }
  virtual Value prev_frame() const { return Value(); }
  virtual void update_velocity(const Value& v) { (void)v; }
  virtual void ctrl_reset_key_list() {}
  virtual Value data_id() const { return Value(); }
  virtual Value shaking() const { return Value(); }
  virtual void handle_ground_velocity_decay(double factor) { (void)factor; }
  virtual Value fuse_bys() const { return Value(); }
  virtual void ref_set_velocity(const Value& who, const Value& x, const Value& y,
                                const Value& z) {
    (void)who;
    (void)x;
    (void)y;
    (void)z;
  }
  virtual void dismiss_fusion(const Value& frame_id) { (void)frame_id; }
  virtual Value defend_value_max() const { return Value(); }
  virtual void set_defend_value(const Value& v) { (void)v; }
  virtual Value resting_max() const { return Value(); }
  virtual void set_resting(const Value& v) { (void)v; }
  virtual void set_throwinjury(const Value& v) { (void)v; }
  virtual Value data_indexes_critical_hit() const { return Value(); }
  virtual Value lying_a_count() const { return Value(); }
  virtual void set_lying_a_count(const Value& v) { (void)v; }
  virtual Value lying_d_count() const { return Value(); }
  virtual void set_lying_d_count(const Value& v) { (void)v; }
  virtual Value lying_c_count() const { return Value(); }
  virtual void set_lying_c_count(const Value& v) { (void)v; }
  virtual Value toughness_max() const { return Value(); }
  virtual void set_toughness_resting(const Value& v) { (void)v; }
  virtual void set_hp_max(const Value& v) { (void)v; }
  virtual Value dead_join() const { return Value(); }
  virtual void set_dead_join(const Value& v) { (void)v; }
  virtual Value dead_gone() const { return Value(); }
  virtual Value reserve() const { return Value(); }
  virtual void set_reserve(const Value& v) { (void)v; }
  virtual Value wakeup_invuln() const { return Value(); }
  virtual void set_wakeup_invuln(const Value& v) { (void)v; }
  virtual void set_invulnerable(const Value& v) { (void)v; }
  virtual void set_blinking(const Value& v) { (void)v; }
  virtual void blink_and_respawn(const Value& duration) { (void)duration; }
  virtual void blink_and_gone(const Value& duration) { (void)duration; }
  virtual Value world_puppets() const { return Value(); }
  virtual void world_etc(double x, double y, double z, const std::u16string& kind) {
    (void)x;
    (void)y;
    (void)z;
    (void)kind;
  }
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
  virtual Value motionless() const { return Value(); }
  virtual void set_motionless(const Value& v) { (void)v; }
  virtual bool has_bearer() const { return false; }
  virtual Value bearer_motionless() const { return Value(); }
  virtual void set_bearer_motionless(const Value& v) { (void)v; }
  virtual void world_callbacks_call(const std::u16string& name) { (void)name; }
  virtual void handle_ground_velocity_decay() {}
  virtual Value data_indexes_default() const { return Value(); }
  virtual Value data_indexes_landing_1() const { return Value(); }
  virtual Value data_indexes_landing_2() const { return Value(); }
  virtual Value data_indexes_heavy_obj_walk() const { return Value(); }
  virtual bool has_data_indexes() const { return false; }
  virtual Value data_indexes_on_ground() const { return Value(); }
  virtual Value data_indexes_throwings() const { return Value(); }
  virtual Value data_indexes_throw_on_ground() const { return Value(); }
  virtual Value data_indexes_just_on_ground() const { return Value(); }
  virtual Value data_base() const { return Value(); }
  virtual Value base_type() const { return Value(); }
  virtual Value drop_hurted() const { return Value(); }
  virtual void set_drop_hurted(const Value& v) { (void)v; }
  virtual void set_dropping(bool v) { (void)v; }
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
