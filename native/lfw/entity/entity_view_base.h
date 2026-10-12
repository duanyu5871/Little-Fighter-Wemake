#pragma once

#include <string>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

// `buff::IBuffEntity` 面的共享实现。`EntityCollisionView`（collision/ 层的实体视图）与
// `EntityStateView`（`Entity::state_view_`）原先各写一份**逐字相同**的转发（29/30 个共同
// 方法），4L/4BW 两刀都为此改了两处 ⇒ 上收到这里，这一面只写一遍。
//
// 两条派生链各自的其它窄接口（`state::IStateEntity` / `collision::*`）仍由各视图实现；
// `buff::IBuffEntity` 采**虚继承**（`IStateEntity` 与 `EntityViewBase` 两条路汇合到同一个
// 子对象）。冲突处理：`EntityCollisionView` 里与碰撞家族**同名**的那些方法（`hp` /
// `state` / `set_velocity` / …）必须在派生类保留一行转发——一个类里的同名同签名函数只能
// 覆盖「继承闭包里所有基类」中同签名的那一个虚槽；两条独立链（`IBuffEntity` 与
// `IHandlerEntity` 等）各要各自的 final overrider，跨链无法靠一条声明同时顶上。
class EntityViewBase : public virtual buff::IBuffEntity {
 public:
  explicit EntityViewBase(Entity& e) : _e(e) {}

  // --- `buff::IBuffEntity` 纯虚 ---
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

  // --- `buff::IBuffEntity` 的默认虚（原先由两视图分头实现） ---
  Value data() const override;
  Value state() const override;
  Value mp() const override;
  void set_mp(const Value& v) override;
  Value mp_max() const override;
  Value dataset(const std::u16string& key) const override;
  Value hp() const override;
  Value hp_r() const override;
  void set_hp(const Value& v) override;
  void set_hp_r(const Value& v) override;
  void set_toughness(const Value& v) override;
  Value data_type() const override;
  double velocity_y() const override;
  void set_velocity(const Value& x, const Value& y, const Value& z) override;
  Value team() const override;
  void set_team(const Value& v) override;
  Value data_indexes_in_the_skys() const override;
  Value marks_get(const std::u16string& key) const override;
  void marks_set(const std::u16string& key, const std::u16string& value) override;
  bool marks_delete(const std::u16string& key) override;

 protected:
  Entity& _e;
};

}
