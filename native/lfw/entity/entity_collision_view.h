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
class EntityActionView;
class EntityHandlerView;
class EntityWeaponView;

// 视图要用到「另一个实体」的视图（`catcher` / `catching` / `bearer`）与 `lfw.mt`
// （`spark_point`）。宿主保证同一个实体每次拿到**同一个**视图对象（指针稳定）。
class ICollisionViewHost {
 public:
  virtual ~ICollisionViewHost() = default;
  virtual EntityHandlerView* handler_view(Entity* e) = 0;
  virtual EntityWeaponView* weapon_view(Entity* e) = 0;
  virtual EntityActionView* action_view(Entity* e) = 0;
  virtual buff::IBuffEntity* buff_view(Entity* e) = 0;
  virtual Entity* entity_by_id(const std::u16string& id) = 0;
  virtual double mt_range(double min, double max) = 0;
  virtual void mt_mark(const std::u16string& mark) = 0;
};

// TS 里碰撞的两边就是 `Entity` 本身（`collision.attacker: Entity`），端口的
// `collision/*` 收的是窄接口 ⇒ 这层把 `Entity` 转发上去（与 `EntityStateView` 同一套路）。
//
// 一个类**不能**同时实现 `IFallEntity` 与 `IWeaponIsHitEntity`：两条继承链各自从
// `IHandlerEntity` 派生且都是非虚继承 ⇒ 会出现两个 `IHandlerEntity` 子对象（对
// `IHandlerEntity&` 的转换二义）。所以按继承链分三个视图：
//   `EntityHandlerView` : `INdbdyDefendEntity`（含 `INbdyNormalEntity` / `IFallEntity` /
//                         `IHandlerEntity`）
//   `EntityWeaponView`  : `IWeaponIsHitEntity`（含 `IHandlerEntity`）
//   `EntityActionView`  : `IActionEntity` / `IH3Entity` / `IH4Entity` / `IFrozenEntity` /
//                         `IHealingEntity` / `buff::IBuffEntity`
class EntityHandlerView : public collision::INdbdyDefendEntity {
 public:
  EntityHandlerView(Entity& e, ICollisionViewHost& host) : _e(e), _host(host) {}
  Entity& entity() const { return _e; }

  // --- `buff::IBuffEntity` 不需要的那部分 `IHandlerEntity` ---
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
  double velocity_x() const override;
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

 private:
  Entity& _e;
  ICollisionViewHost& _host;
};

class EntityWeaponView : public collision::IWeaponIsHitEntity {
 public:
  EntityWeaponView(Entity& e, ICollisionViewHost& host) : _e(e), _host(host) {}
  Entity& entity() const { return _e; }

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

  bool has_bearer() const override;
  void set_dropping(bool v) override;
  Value base_type() const override;
  Value facing() const override;
  Value team() const override;
  void set_team(const Value& v) override;
  Value data_id() const override;
  Value data_indexes_throwings() const override;
  Value data_indexes_in_the_skys() const override;
  void leave_ground() override;

 private:
  Entity& _e;
  ICollisionViewHost& _host;
};

class EntityActionView : public buff::IBuffEntity,
                         public collision::IActionEntity,
                         public collision::IH3Entity,
                         public collision::IH4Entity,
                         public collision::IFrozenEntity,
                         public collision::IHealingEntity {
 public:
  EntityActionView(Entity& e, ICollisionViewHost& host) : _e(e), _host(host) {}
  Entity& entity() const { return _e; }

  // --- `buff::IBuffEntity` ---
  const std::u16string& id() const override;
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
  Value data_indexes_in_the_skys() const override;
  Value marks_get(const std::u16string& key) const override;
  void marks_set(const std::u16string& key, const std::u16string& value) override;
  bool marks_delete(const std::u16string& key) override;

  // --- `IActionEntity` ---
  Value data() const override;
  Value velocity_x() const override;
  void set_velocity_x(const Value& v) override;
  Value facing() const override;
  void set_facing(const Value& v) override;
  Value team() const override;
  void set_team(const Value& v) override;
  Value hp() const override;
  void set_hp(const Value& v) override;
  Value hp_r() const override;
  void set_hp_r(const Value& v) override;
  Value hp_max() const override;
  Value mp() const override;
  void set_mp(const Value& v) override;
  Value mp_max() const override;
  bool is_bot_ctrl() const override;
  Value src_emitter() const override;
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
  void enter_frame(const Value& info) override;
  void transform(const Value& data) override;
  void set_prop(const std::u16string& name, const Value& v) override;
  buff::IBuffEntity* buff_entity() override;

  // --- `IH3Entity` ---
  void velocity(double& x, double& y, double& z) const override;
  void set_velocity(const Value& x, const Value& y, const Value& z) override;
  bool has_bearer() const override;
  Value base_type() const override;
  Value state() const override;
  Value data_in_the_skys_first() const override;
  Value data_base_hit_sounds() const override;
  Value armor() const override;
  Value toughness() const override;
  void set_toughness(const Value& v) override;
  Value toughness_max() const override;
  Value itr_fall(const Value& itr) const override;
  Value dataset(const std::u16string& key) const override;
  void set_motionless(const Value& v) override;
  void set_shaking(const Value& v) override;
  void enter_frame_by_id(const Value& id) override;
  void play_sound(const Value& sounds) override;

  // --- `IH4Entity` ---
  Value frame_id() const override;
  Value data_indexes_throwings() const override;
  Value arest() const override;
  void set_arest(const Value& v) override;
  void set_dropping(bool v) override;

  // --- `IFrozenEntity` ---
  Value group() const override;
  Value frame() const override;
  double position_x() const override;
  double position_y() const override;
  double position_z() const override;
  bool spawn(const Value& opoint, const Value& face) override;

  // --- `IHealingEntity`（`id()` / `dataset()` / `buff_entity()` 已在上面）---

 private:
  Entity& _e;
  ICollisionViewHost& _host;
};

}