#pragma once

#include <string>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

// `Entity` 面的共享实现。`EntityCollisionView`（collision/ 层的实体视图）与
// `EntityStateView`（`Entity::state_view_`）原先各写一份**逐字相同**的转发（29/30 个共同
// 方法），4L/4BW 两刀都为此改了两处 ⇒ 上收到这里，这一面只写一遍。
//
// 切片 4BY 起本类是**纯转发**：`buff::IBuffEntity` 退役、`Entity` 自己就是实体面 ⇒ 它不再
// 实现任何接口。两个视图各用自己的窄接口（`state::IStateEntity` / `collision::*`），需要的
// 那几条在派生类里**显式重写**再转到这里 —— 同名同签名的两条独立虚链各要各自的 final
// overrider，一条声明顶不了两个虚槽。
class EntityViewBase {
 public:
  explicit EntityViewBase(Entity& e) : _e(e) {}

  // --- 实体面：`Entity` 自己的那批（原先由窄接口承载）---
  const std::u16string& id() const;
  void position(double& x, double& y, double& z) const;
  void set_position(double x, double y, double z);
  double frame_centery() const;
  double frame_height() const;
  double frame_pic_h() const;
  void set_frame(const Value& info);
  void buffs_set(const std::u16string& key, buff::Buff* b);
  void buffs_delete(const std::u16string& key);
  void set_outline_alpha(double v);
  void set_outline_width(double v);
  void set_outline_color(const std::u16string& v);
  void enter_frame_by_id(const std::u16string& id);
  void attach(bool on);

  // --- 实体面的其余条目（原先由两视图分头实现） ---
  Value data() const;
  Value state() const;
  Value mp() const;
  void set_mp(const Value& v);
  Value mp_max() const;
  Value dataset(const std::u16string& key) const;
  Value hp() const;
  Value hp_r() const;
  void set_hp(const Value& v);
  void set_hp_r(const Value& v);
  void set_toughness(const Value& v);
  Value data_type() const;
  double velocity_y() const;
  void set_velocity(const Value& x, const Value& y, const Value& z);
  Value team() const;
  void set_team(const Value& v);
  Value data_indexes_in_the_skys() const;
  Value marks_get(const std::u16string& key) const;
  void marks_set(const std::u16string& key, const std::u16string& value);
  bool marks_delete(const std::u16string& key);

 protected:
  Entity& _e;
};

}
