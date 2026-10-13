#pragma once

#include <functional>
#include <string>

#include "lfw/core/value.h"

namespace lfw {

class Entity;

namespace collision {

// 切片 4CE：`IFrozenEntity` 已退役 ⇒ 缝里直接是 `lfw::Entity`。
struct BallFrozenEnv {
  std::function<bool(const Entity& e)> is_ball;
  std::function<bool(const Entity& e)> is_fighter;
};

const BallFrozenEnv& ball_frozen_env();
void set_ball_frozen_env(const BallFrozenEnv& env);

bool handle_ball_frozen(Entity& a, Entity& v, const Value& itr);

}

}
