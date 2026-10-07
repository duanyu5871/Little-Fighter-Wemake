#pragma once

#include <functional>
#include <string>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IActionEntity {
  virtual ~IActionEntity() = default;
  virtual const std::u16string& id() const = 0;
  virtual Value data() const = 0;
  virtual Value data_type() const = 0;
  // 必须是 `double`：`EntityCollisionView` 要同时实现它与 `IFallEntity::velocity_x()`
  // （`double`），同名同参不同返回类型没法共用一个重写（见 `entity_collision_view.h`）。
  virtual double velocity_x() const = 0;
  virtual void set_velocity_x(const Value& v) = 0;
  virtual Value facing() const = 0;
  virtual void set_facing(const Value& v) = 0;
  virtual Value team() const = 0;
  virtual void set_team(const Value& v) = 0;
  virtual Value hp() const = 0;
  virtual void set_hp(const Value& v) = 0;
  virtual Value hp_r() const = 0;
  virtual void set_hp_r(const Value& v) = 0;
  virtual Value hp_max() const = 0;
  virtual Value mp() const = 0;
  virtual void set_mp(const Value& v) = 0;
  virtual Value mp_max() const = 0;
  virtual bool is_bot_ctrl() const = 0;
  virtual Value src_emitter() const = 0;
  virtual Value emitter() const = 0;
  virtual IActionEntity* bearer() = 0;
  virtual Value fuse_bys() const = 0;
  virtual void set_fuse_bys(const Value& v) = 0;
  virtual void set_dismiss_data(const Value& v) = 0;
  virtual void set_dismiss_time(const Value& v) = 0;
  virtual void set_invisible(double v) = 0;
  virtual void set_motionless(double v) = 0;
  virtual void set_invulnerable(double v) = 0;
  virtual void play_sound(const Value& sounds, const Value& pos) = 0;
  virtual void enter_frame(const Value& info) = 0;
  virtual void transform(const Value& data) = 0;
  virtual void set_prop(const std::u16string& name, const Value& v) = 0;
  virtual buff::IBuffEntity* buff_entity() = 0;
};

struct ActionEnv {
  std::function<double()> mt_int;
  std::function<void(const std::u16string& mark)> mt_set_mark;
  std::function<bool(const std::u16string& oid, Value& out)> find_data;
  std::function<void(const std::u16string& msg)> broadcast;
  std::function<void(const std::u16string& msg)> alert;
  std::function<IActionEntity*(const std::u16string& id)> find_entity;
  std::function<bool(const IActionEntity& a, const IActionEntity& v)> is_ally;
  std::function<const buff::BuffEnv*()> buff_env;
};

Value run_action(const ActionEnv& env, const std::u16string& type, const Value& action,
                 IActionEntity& attacker, IActionEntity& victim, const Value& injury,
                 const Value& real_injury);

}
}
