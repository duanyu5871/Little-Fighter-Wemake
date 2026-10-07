#pragma once

#include <string>

#include "lfw/buff/buff.h"
#include "lfw/collision/action_handlers.h"
#include "lfw/collision/ball_frozen.h"
#include "lfw/collision/handlers2.h"
#include "lfw/collision/handlers3.h"
#include "lfw/collision/handlers4.h"
#include "lfw/collision/healing.h"
#include "lfw/collision/n_bdy_defend.h"
#include "lfw/collision/n_bdy_normal.h"
#include "lfw/collision/weapon_is_hit.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;
class EntityCollisionView;

// 视图要用到「另一个实体」的视图（`catcher` / `catching` / `bearer`）与 `lfw.mt`
// （`spark_point`）。宿主保证同一个实体每次拿到**同一个**视图对象（指针稳定）。
class ICollisionViewHost {
 public:
  virtual ~ICollisionViewHost() = default;
  virtual EntityCollisionView* handler_view(Entity* e) = 0;
  virtual EntityCollisionView* weapon_view(Entity* e) = 0;
  virtual EntityCollisionView* action_view(Entity* e) = 0;
  virtual buff::IBuffEntity* buff_view(Entity* e) = 0;
  virtual Entity* entity_by_id(const std::u16string& id) = 0;
  // 窄接口 → 被包的 `Entity`。合并三视图之后 `IHandlerEntity` 成了**虚基类** ⇒ `-fno-rtti`
  // 下不能再像非虚基类那样 `static_cast` 回本类（`C2635`）⇒ 宿主建视图时登记反向表，这里查回。
  virtual Entity* entity_of_handler(const collision::IHandlerEntity* v) = 0;
  virtual double mt_range(double min, double max) = 0;
  virtual void mt_mark(const std::u16string& mark) = 0;
};

// TS 里碰撞的两边就是 `Entity` 本身（`collision.attacker: Entity`），端口的
// `collision/*` 收的是窄接口 ⇒ 这层把 `Entity` 转发上去（与 `EntityStateView` 同一套路）。
//
// 原来按继承链分三个视图（`EntityHandlerView` / `EntityWeaponView` / `EntityActionView`），
// 现在合并成这一个：它们本来就只是把同一个 `Entity` 转发给不同的窄接口，分开只是因为
// `IFallEntity` 与 `IWeaponIsHitEntity` 各自从 `IHandlerEntity` **非虚**派生 ⇒ 一个类同时
// 实现两者会拿到两个 `IHandlerEntity` 子对象（对 `IHandlerEntity&` 的转换二义）。现已把那两
// 处直接派生改成 `virtual IHandlerEntity`（`collision/fall.h` / `collision/weapon_is_hit.h`），
// 菱形消失，于是本类一次实现三个视图的窄接口：
//   `INdbdyDefendEntity`（含 `INbdyNormalEntity` / `IFallEntity` / `IHandlerEntity`）
//   `IWeaponIsHitEntity`（含 `IHandlerEntity`）
//   `IActionEntity` / `IH3Entity` / `IH4Entity` / `IFrozenEntity` / `IHealingEntity` /
//   `buff::IBuffEntity`
// 两处代价（都不改行为）：
//   1. `IHandlerEntity` 是虚基类 ⇒ 不能再 `static_cast` 回本类（见
//      `ICollisionViewHost::entity_of_handler`）；
//   2. `IFallEntity::velocity_x()`（`double`）与 `IActionEntity::velocity_x()` 同名同参 ⇒
//      后者改成 `double`（`collision/action_handlers.h`），本类只写一个重写（取经典链
//      `IFallEntity` 那条的 `double` 语义）。
class EntityCollisionView : public collision::INdbdyDefendEntity,
                            public collision::IWeaponIsHitEntity,
                            public collision::IActionEntity,
                            public collision::IH3Entity,
                            public collision::IH4Entity,
                            public collision::IFrozenEntity,
                            public collision::IHealingEntity,
                            public buff::IBuffEntity {
 public:
  EntityCollisionView(Entity& e, ICollisionViewHost& host) : _e(e), _host(host) {}
  Entity& entity() const { return _e; }

  // --- `IHandlerEntity`（`buff::IBuffEntity` 不需要的那部分；两条链共用这一份）---
  const std::u16string& id() const override;
  Value hp() const override;
  void set_hp(const Value& v) override;
  Value hp_r() const override;
  void set_hp_r(const Value& v) override;
  void set_toughness(const Value& v) override;
  Value data() const override;
  Value data_indexes_ice() const override;
  Value data_base_hit_sounds() const override;
  Value dataset(const std::u16string& key) const override;
  bool marks_has(const std::u16string& kind) const override;
  bool catching() const override;
  void set_catching(collision::IHandlerEntity* v) override;
  Value catch_time_max() const override;
  void set_catch_time(const Value& v) override;
  collision::IHandlerEntity* catcher() const override;
  void set_catcher(collision::IHandlerEntity* v) override;
  Value resting() const override;
  void set_resting(const Value& v) override;
  Value fall_value() const override;
  void set_fall_value(const Value& v) override;
  Value fall_value_max() const override;
  Value defend_value() const override;
  void set_defend_value(const Value& v) override;
  Value defend_value_max() const override;
  Value itr_fall(const Value& itr) const override;
  Value src_emitter() const override;
  Value shaking() const override;
  void set_shaking(const Value& v) override;
  Value motionless() const override;
  void set_velocity(const Value& x, const Value& y, const Value& z) override;
  void enter_frame(const Value& info) override;
  void enter_frame_by_id(const Value& id) override;
  void play_sound(const Value& sounds) override;
  buff::IBuffEntity* buff_entity() override;

  // --- `IFallEntity` ---
  Value facing() const override;
  double velocity_x() const override;  // 同时重写 `IActionEntity::velocity_x()`
  void spark_point(const collision::Cube& a, const collision::Cube& b, double& x, double& y,
                   double& z) override;
  Value data_indexes_fire() const override;
  Value data_indexes_critical_hit() const override;
  Value holding_base_type() const override;
  void drop_holding() override;

  // --- `INbdyNormalEntity` ---
  Value state() const override;
  double position_y() const override;
  double ground_y() const override;
  Value data_indexes_dizzy() const override;
  Value data_indexes_grand_injured() const override;
  Value data_indexes_injured() const override;
  Value cpoint_backhurtact() const override;
  Value cpoint_fronthurtact() const override;

  // --- `INdbdyDefendEntity` ---
  Value defend_ratio() const override;

  // --- `IWeaponIsHitEntity` ---
  bool has_bearer() const override;
  void set_dropping(bool v) override;
  Value base_type() const override;
  Value team() const override;
  void set_team(const Value& v) override;
  Value data_id() const override;
  Value data_indexes_throwings() const override;
  Value data_indexes_in_the_skys() const override;
  void leave_ground() override;

  // --- `buff::IBuffEntity` ---
  void position(double& x, double& y, double& z) const override;
  void set_position(double x, double y, double z) override;
  double frame_centery() const override;
  double frame_height() const override;
  double frame_pic_h() const override;
  void set_frame(const Value& info) override;
  void buffs_set(const std::u16string& key, buff::Buff* b) override;
  void buffs_delete(const std::u16string& key) override;
  void set_outline_alpha(double v) override;
  void set_outline_width(double v) override;
  void set_outline_color(const std::u16string& v) override;
  void enter_frame_by_id(const std::u16string& id) override;
  void attach(bool on) override;
  Value data_type() const override;
  Value marks_get(const std::u16string& key) const override;
  void marks_set(const std::u16string& key, const std::u16string& value) override;
  bool marks_delete(const std::u16string& key) override;

  // --- `IActionEntity` ---
  void set_velocity_x(const Value& v) override;
  void set_facing(const Value& v) override;
  Value hp_max() const override;
  Value mp() const override;
  void set_mp(const Value& v) override;
  Value mp_max() const override;
  bool is_bot_ctrl() const override;
  Value emitter() const override;
  collision::IActionEntity* bearer() override;
  Value fuse_bys() const override;
  void set_fuse_bys(const Value& v) override;
  void set_dismiss_data(const Value& v) override;
  void set_dismiss_time(const Value& v) override;
  void set_invisible(double v) override;
  void set_motionless(double v) override;
  void set_invulnerable(double v) override;
  void play_sound(const Value& sounds, const Value& pos) override;
  void transform(const Value& data) override;
  void set_prop(const std::u16string& name, const Value& v) override;

  // --- `IH3Entity` ---
  void velocity(double& x, double& y, double& z) const override;
  Value data_in_the_skys_first() const override;
  Value armor() const override;
  Value toughness() const override;
  Value toughness_max() const override;
  void set_motionless(const Value& v) override;

  // --- `IH4Entity` ---
  Value frame_id() const override;
  Value arest() const override;
  void set_arest(const Value& v) override;

  // --- `IFrozenEntity` ---
  Value group() const override;
  Value frame() const override;
  double position_x() const override;
  double position_z() const override;
  bool spawn(const Value& opoint, const Value& face) override;

  // `IHealingEntity` 只要 `id()` / `dataset()` / `buff_entity()` —— 都在上面。

 private:
  Entity& _e;
  ICollisionViewHost& _host;
};

}
