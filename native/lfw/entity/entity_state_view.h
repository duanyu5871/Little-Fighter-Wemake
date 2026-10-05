#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/state/state_base.h"

namespace lfw {

class Entity;

// The TS state hooks receive the entity itself (`on_dead?.(this)`), while the port's
// `state::IStateEntity` spells the same members with `Value` signatures that clash
// with `Entity`'s `double` getters (`hp_max()`), so this view forwards between the
// two.  Only members whose `Entity` side is already ported are overridden; everything
// else keeps the `IStateEntity` default, which is the placeholder for the slice that
// owns it (buff wiring, terrain, frame entering).  The `IBuffEntity` pure virtuals
// have to exist no matter what — the ones nothing can answer yet are inert and mark
// themselves as such.
class EntityStateView : public state::IStateEntity {
 public:
  explicit EntityStateView(Entity& e) : _e(e) {}

  // --- `buff::IBuffEntity` pure virtuals --------------------------------------
  const std::u16string& id() const override;
  void position(double& x, double& y, double& z) const override;
  // `this.set_position(x, y, z)` — the World owns positions, so this lands with the
  // terrain / restriction slice.
  void set_position(double x, double y, double z) override;
  // `this.position.x = x` 直写三个字段：`State_Base.on_restrict` 走这条，不能再经过
  // `set_position`（那里会重新调用状态钩子）。
  void assign_position(double x, double y, double z) override;
  // Frame geometry comes from the frame-info helpers (`frame_centery` …), which the
  // AABB / frame-entering slices port.
  double frame_centery() const override;
  double frame_height() const override;
  double frame_pic_h() const override;
  // `this.set_frame(info)` belongs to the enter-frame chain.
  void set_frame(const Value& info) override;
  void buffs_set(const std::u16string& key, buff::Buff* b) override;
  void buffs_delete(const std::u16string& key) override;
  void set_outline_alpha(double v) override;
  void set_outline_width(double v) override;
  void set_outline_color(const std::u16string& v) override;
  // `this.enter_frame_by_id(id)` belongs to the enter-frame chain.
  void enter_frame_by_id(const std::u16string& id) override;
  // `this.attach(...)` needs `world.add_entities`.
  void attach(bool on) override;

  // --- `state::IStateEntity` pure virtuals ------------------------------------
  Value velocity_x() const override;
  Value velocity_z() const override;
  double velocity_y() const override;
  // `e.set_velocity(x, y, z)` — `State_Base.on_restrict` 的速度钳制走这里。
  void set_velocity(const Value& x, const Value& y, const Value& z) override;

  // --- forwarded members ------------------------------------------------------
  Value hp() const override;
  Value hp_r() const override;
  void set_hp_r(const Value& v) override;
  Value hp_max() const override;
  Value mp() const override;
  Value motionless() const override;
  void set_motionless(const Value& v) override;
  Value shaking() const override;
  Value state() const override;
  Value frame_info() const override;
  Value prev_frame() const override;
  bool is_on_ground() const override;
  Value team() const override;
  Value data_type() const override;
  Value jumping_x() const override;
  // `this.dismiss_fusion(frame_id)` — the fusion split (`character_state_falling` 与
  // `Entity.check_fusion_dismissing` 各有一条调用）。
  void dismiss_fusion(const Value& frame_id) override;
  // `this.world.puppets` — the state hooks read it as records (see `Entity::world_puppets`).
  Value world_puppets() const override;
  // 状态代码里的 `e.dataset(key)` / `e.world_dataset(key)`：前者是实体那一串回落
  // （帧 `dataset` → `data.base` → 背景 → 世界），后者只读世界那层。这两条不接线的话
  // 状态钩子读到的全是 `undefined`（`CharacterState_Base.get_caught_end_frame` 就会
  // 把 `-1 * undefined * facing` 写进速度）。
  Value dataset(const std::u16string& key) const override;
  Value world_dataset(const std::u16string& key) const override;
  // `this.facing` / `this.enter_frame(...)` / `this.drop_holding()` /
  // `this.handle_ground_velocity_decay(...)` —— `CharacterState_Base` 的六个钩子要用到
  // 的那几条（其余的 `e.data_indexes_*` / `e.frame_on_landing` / `e.holding_base_type`
  // 等要等数据索引切片接线）。
  Value facing() const override;
  void enter_frame(const Value& frame) override;
  void drop_holding() override;
  void handle_ground_velocity_decay() override;
  void handle_ground_velocity_decay(double factor) override;

 private:
  Entity& _e;
};

}
