#pragma once

#include <functional>
#include <string>

#include "lfw/collision/fall.h"
#include "lfw/core/value.h"

namespace lfw {

class Entity;

namespace collision {

// 切片 4CE：`INbdyNormalEntity` / `IFallEntity` 已退役 ⇒ 缝里直接是 `lfw::Entity`。
struct NbdyNormalEnv {
  std::function<Entity*(const std::u16string& id)> find_entity;
  std::function<bool(const Entity& e)> is_fighter;
  std::function<bool(Collision& c)> is_fall;
  std::function<void(const Value& x, const Value& y, const Value& z, const Value& type)> spark;
  std::function<ItrVelocity(Collision& c)> calc_velocity;
};

void handle_itr_normal_bdy_normal(Collision& c);

const NbdyNormalEnv& nbdy_normal_env();
void set_nbdy_normal_env(const NbdyNormalEnv& env);

}

}
