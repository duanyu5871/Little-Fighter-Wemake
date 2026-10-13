#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/entity/entity_view_base.h"
#include "lfw/state/state_base.h"

namespace lfw {

class Entity;

// TS 的状态钩子收实体本身（`on_dead?.(this)`），端口的 `state::IStateEntity` 把同一批成员
// 拼成 `Value` 签名、与 `Entity` 的 `double` getter（`hp_max()`）错开，这一层负责转译。
// 只有 `Entity` 侧已落地的那部分被 override；其余保持 `IStateEntity` 的默认实现——那是
// 拥有它的那一刀的占位。`Entity` 面（`id` / 帧几何 / 描边 / `attach` …）与
// `EntityCollisionView` 共用 `EntityViewBase`（虚继承汇合到同一个子对象）。
class EntityStateView : public state::IStateEntity, public EntityViewBase {
 public:
  explicit EntityStateView(Entity& e) : EntityViewBase(e) {}

  // --- `state::IStateEntity` pure virtuals ------------------------------------
  // `this.position.x = x` 直写三个字段：`State_Base.on_restrict` 走这条，不能再经过
  // `set_position`（那里会重新调用状态钩子）。
  void assign_position(double x, double y, double z) override;
  double velocity_x() const override;
  Value velocity_z() const override;
  // `this.ground_y` —— 落地判定的读点（`py > this.ground_y`）。
  double ground_y() const override;

  // --- forwarded members ------------------------------------------------------
  Value hp_max() const override;
  Value motionless() const override;
  void set_motionless(const Value& v) override;
  Value shaking() const override;
  Value frame_info() const override;
  Value prev_frame() const override;
  bool is_on_ground() const override;
  Value jumping_x() const override;
  // `this.dismiss_fusion(frame_id)` — the fusion split (`character_state_falling` 与
  // `Entity.check_fusion_dismissing` 各有一条调用）。
  void dismiss_fusion(const Value& frame_id) override;
  // `this.world.puppets` — the state hooks read it as records (see `Entity::world_puppets`).
  Value world_puppets() const override;
  // 状态代码里的 `e.world_dataset(key)`：只读世界那层（`e.dataset(key)` 的实体回落链在
  // `EntityViewBase`）。不接线的话状态钩子读到的是 `undefined`。
  Value world_dataset(const std::u16string& key) const override;
  Entity* entity_self() override { return &_e; }
  // `this.facing` / `this.enter_frame(...)` / `this.drop_holding()` /
  // `this.handle_ground_velocity_decay(...)` —— `CharacterState_Base` 的六个钩子要用到
  // 的那几条（其余的 `e.data_indexes_*` / `e.frame_on_landing` / `e.holding_base_type`
  // 等要等数据索引切片接线）。
  Value facing() const override;
  void enter_frame(const Value& frame) override;
  void drop_holding() override;
  void handle_ground_velocity_decay() override;
  void handle_ground_velocity_decay(double factor) override;

  // --- `state::IStateEntity` 的实体面（切片 4BY 补）--------------------------------
  // `EntityViewBase` 不再借 `buff::IBuffEntity` 的虚槽实施（那条接口已退役），于是
  // `IStateEntity` 自带的默认体会赢（`hp()` → `undefined`、`position()` → 全 0 …）。
  // 4BX 时这些槽的 final overrider 就是 `EntityViewBase` ⇒ 在这里逐条显式重写回去。
  void position(double& x, double& y, double& z) const override;
  void set_position(double x, double y, double z) override;
  void enter_frame_by_id(const std::u16string& id) override;
  void set_outline_color(const std::u16string& v) override;
  Value data() const override;
  Value data_type() const override;
  Value dataset(const std::u16string& key) const override;
  Value data_indexes_in_the_skys() const override;
  Value hp() const override;
  Value hp_r() const override;
  void set_hp(const Value& v) override;
  void set_hp_r(const Value& v) override;
  void set_toughness(const Value& v) override;
  Value mp() const override;
  void set_mp(const Value& v) override;
  Value mp_max() const override;
  Value state() const override;
  Value team() const override;
  void set_team(const Value& v) override;
  void set_velocity(const Value& x, const Value& y, const Value& z) override;
  double velocity_y() const override;
};

}
