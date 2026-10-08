#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/helper/randoming.h"

namespace lfw {

class Entity;
class MersenneTwister;

namespace controller {
class BaseController;
}

namespace helper {

// TS `helper/EntitiesHelper.ts` 等四个 helper 依赖的 `lfw` 面（`LFW` 未移植 ⇒ 宿主缝）。
// 与 `IWorldLfw` 同套路：逐一对应一条 TS 表达式，写在注释里。
class IHelperLfw {
 public:
  virtual ~IHelperLfw() = default;
  // `lfw.mt`
  virtual MersenneTwister& mt() = 0;
  // `lfw.world.entities` / `lfw.world.ghosts`
  virtual const std::vector<Entity*>& world_entities() = 0;
  virtual const std::vector<Entity*>& world_ghosts() = 0;
  // `lfw.world.del_entities(list)`
  virtual void del_entities(const std::vector<Entity*>& list) = 0;
  // `lfw.factory.create_entity(lfw.world, data)`；造不出 ⇒ nullptr（TS 的 undefined）
  virtual Entity* create_entity(const Value& data) = 0;
  // `lfw.factory.create_ctrl(data.id, "", entity)`
  virtual controller::BaseController* create_ctrl(const Value& oid,
                                                  const std::u16string& player_id,
                                                  Entity* entity) = 0;
  // `lfw.datas.find_fighter(id)` / `lfw.datas.find_weapon(id)`；找不到 ⇒ nullptr
  virtual const Value* find_fighter(const Value& id) = 0;
  virtual const Value* find_weapon(const Value& id) = 0;
  // `lfw.datas.fighters` / `lfw.datas.weapons`
  virtual const std::vector<Value>& fighters() = 0;
  virtual const std::vector<Value>& weapons() = 0;
  // `lfw.new_team`（getter：每次读都自增）
  virtual std::u16string new_team() = 0;
  // `lfw.random_entity_info(entity)`
  virtual void random_entity_info(Entity& e) = 0;
};

// TS `ObjectsHelper`。
class ObjectsHelper {
 public:
  static constexpr const char* TAG = "ObjectsHelper";

  explicit ObjectsHelper(IHelperLfw& lfw);

  IHelperLfw& lfw() const { return *_lfw; }
  RandomingT<Value>& team_randoming() { return _team_randoming; }

  virtual std::vector<Entity*> all() const;
  Entity* a() const { return at(0); }
  Entity* b() const { return at(1); }
  Entity* at(double idx) const;

  // `add(data, num = 1, team?)`；`team` 用「空指针 = undefined」表达。
  virtual std::vector<Entity*> add(const Value& data, double num = 1,
                                   const std::u16string* team = nullptr);
  void del_all();

 protected:
  IHelperLfw* _lfw;
  RandomingT<Value> _team_randoming;
};

}  // namespace helper
}  // namespace lfw
