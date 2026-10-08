#pragma once

#include "lfw/entity/entity.h"
#include "lfw/stage/item.h"

namespace lfw {
namespace stage {

// `Entity` 当 `stage::IItemEntity` 用时的适配器（`Entity` 的 `attach()` 带默认参数、返回
// 引用，签名与 `IItemEntity` 对不上；`team()` 的类型也不同）⇒ 端口给一层薄包装。
// 生命周期：实体由工厂/世界持有（回收进 Graves 池不析构）⇒ 适配器跟着实体活；调用方
// （`LFW::create_entity_with_bot`）缓存一份。
class EntityItem : public IItemEntity {
 public:
  explicit EntityItem(Entity& e) : _e(e) {}

  Value ref() const override { return _e.ref(); }
  const Value& data() const override { return _e.data(); }
  Value team() const override { return Value(_e.team()); }
  Callbacks& callbacks() override { return _e.callbacks; }

  void set_outline_color(const Value& v) override { _e.set_outline_color(to_string(v)); }
  void set_stat_bar(double v) override { _e.stat_bar = v; }
  void set_wakeup_invuln(double v) override { _e.wakeup_invuln = v; }
  void set_dead_gone(double v) override { _e.dead_gone = v; }
  void set_reserve(const Value& v) override { _e.set_reserve(to_number(v)); }
  void set_hp(double v) override { _e.set_hp(v); }
  void set_hp_r(double v) override { _e.set_hp_r(v); }
  void set_hp_max(double v) override { _e.set_hp_max(v); }
  void set_mp(double v) override { _e.set_mp(v); }
  void set_mp_max(double v) override { _e.set_mp_max(v); }
  void set_name(const Value& v) override { _e.set_name(v); }
  void set_team(const Value& v) override { _e.set_team(to_string(v)); }
  void set_facing(const Value& v) override { _e.facing = to_number(v); }
  void set_dead_join(Value v) override { _e.dead_join = std::move(v); }
  void set_position(const Value& x, const Value& y, const Value& z) override {
    _e.set_position(x, y, z);
  }
  void attach() override { _e.attach(); }
  void enter_frame_by_id(const Value& id) override { _e.enter_frame_by_id(id); }
  void enter_frame(const Value& nf) override { _e.enter_frame(nf); }

  Entity& entity() const { return _e; }

 private:
  Entity& _e;
};

}  // namespace stage
}  // namespace lfw
