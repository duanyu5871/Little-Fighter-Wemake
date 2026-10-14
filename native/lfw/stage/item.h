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
class Entity;

namespace stage {

class Stage;

// TS 里 `Item` 直接吃活实体；切片 4CV（ck8）：`IItemEntity` 缝退役 ⇒ 真 `Entity`。

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
  const std::vector<Entity*>& objects() const { return _objects; }
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
    Entity* e = nullptr;
    Callbacks::Remover off_team;
    Callbacks::Remover off_dead;
  };
  void forget(Entity* e);

  Stage* _stage = nullptr;
  Value _phase;
  Value _info;
  std::vector<Entity*> _objects;
  std::vector<Watch> _watches;
  Times _end_delay;
  bool _released = false;
  bool _is_fighter = false;
};

}
}
