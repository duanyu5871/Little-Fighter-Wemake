#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"
#include "lfw/helper/randoming.h"
#include "lfw/utils/times.h"

namespace lfw {

class MersenneTwister;

namespace stage {

// TS 里 `Item` 直接吃 `Entity`；端口沿用既有的窄缝写法（同 `IStageEntity`）：
// 把 `Item` 真正用到的成员收成接口，宿主（`World` 那一刀）再实现它。
class IItemEntity {
 public:
  virtual ~IItemEntity() = default;
  virtual Value ref() const = 0;          // `is_fighter(e)` / `is_weapon(e)` 的入参
  virtual const Value& data() const = 0;  // `e.data.base.name`
  virtual Value team() const = 0;         // `Stage::kill_*` 的 `e.team === stage.team`
  virtual Callbacks& callbacks() = 0;     // `e.callbacks.add` / `del`

  virtual void set_outline_color(const Value&) = 0;
  virtual void set_stat_bar(double v) = 0;
  virtual void set_wakeup_invuln(double v) = 0;
  virtual void set_dead_gone(double v) = 0;
  virtual void set_reserve(const Value&) = 0;
  virtual void set_hp(double v) = 0;
  virtual void set_hp_r(double v) = 0;
  virtual void set_hp_max(double v) = 0;
  virtual void set_mp(double v) = 0;
  virtual void set_mp_max(double v) = 0;
  virtual void set_name(const Value&) = 0;
  virtual void set_team(const Value&) = 0;
  virtual void set_facing(const Value&) = 0;
  virtual void set_dead_join(Value) = 0;
  virtual void set_position(const Value& x, const Value& y, const Value& z) = 0;
  virtual void attach() = 0;
  virtual void enter_frame_by_id(const Value&) = 0;
  virtual void enter_frame(const Value&) = 0;
};

// `Item` 用到的宿主面：`Stage`（`far` / `near` / `team` / `all_boss_dead` /
// `world.dataset.difficulty` / `lfw.mt` / `lfw.datas` / `lfw.factory`）。
// 切片 4CS（ck4）：`IItemHost` 退役 ⇒ `Item` 直接持真 `Stage*`（这里只前置声明，
// `item.cpp` include `stage.h`；反过来 `stage.h` include 本头，所以不能在这边 include）。
class Stage;

// TS `stage/Item.ts`：舞台物件（按 `times` / `end_delay` 刷怪，死亡或换队就出列）。
class Item {
 public:
  using RandomingOfItems = RandomingT<std::shared_ptr<Randoming>>;

  Item(Stage* stage, Value phase, Value info);
  // TS 里回调会一直挂在实体上（JS 对象不会被析构，实体还强引用着那个 `Item`）；
  // 端口必须在析构时摘掉，否则实体会拿着**悬空的 `this`**（本刀踩过）。
  ~Item();

  Stage* stage() const { return _stage; }
  const Value& phase() const { return _phase; }
  const Value& info() const { return _info; }
  const std::vector<IItemEntity*>& objects() const { return _objects; }
  const Times& end_delay() const { return _end_delay; }
  bool released() const { return _released; }
  bool is_fighter() const { return _is_fighter; }

  // TS 里 `times` / `data` / `randoming` 是公开可变字段。
  std::optional<double> times;
  Value data;
  std::shared_ptr<RandomingOfItems> randoming;

  void update();
  bool spawn();
  void release();

 private:
  // TS 的 `entity_callback` 是**一个**共享对象（`on_team_changed` + `on_dead`）；端口按实体各建一份
  // `on()` 监听 —— 效果等价（`objects.delete(e)` + 不再监听 e），只是身份与 `del(对象)` 的写法不同。
  struct Watch {
    IItemEntity* e = nullptr;
    Callbacks::Remover off_team;
    Callbacks::Remover off_dead;
  };
  void forget(IItemEntity* e);

  Stage* _stage = nullptr;
  Value _phase;
  Value _info;
  std::vector<IItemEntity*> _objects;
  std::vector<Watch> _watches;
  Times _end_delay;
  bool _released = false;
  bool _is_fighter = false;
};

}
}
