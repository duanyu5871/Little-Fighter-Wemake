#pragma once

#include <functional>
#include <string>

#include "lfw/collision/handlers2.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

namespace collision {

// 切片 4CE：碰撞族不再有自己的「实体窄接口」（`IFallEntity` / `IHandlerEntity` / … 全退役）——
// TS 的两边本来就是 `Entity`（`collision.attacker: Entity`），端口那套只把 `Entity` 转发一遍
// （实现见已删除的 `entity/entity_collision_view.*`）。缝里现在直接是 `lfw::Entity`。
struct FallEnv {
  std::function<Entity*(const std::u16string& id)> find_entity;
  std::function<bool(const Entity& e)> is_fighter;
  std::function<void(const Value& x, const Value& y, const Value& z, const Value& type)> spark;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_fall(Collision& c);

const FallEnv& fall_env();
void set_fall_env(const FallEnv& env);

}

}
