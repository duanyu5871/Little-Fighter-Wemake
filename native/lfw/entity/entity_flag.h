#pragma once

#include <cstdint>
#include <string>

#include "lfw/core/js_num.h"
#include "lfw/defines/hit_flag.h"

namespace lfw {

// `a.get_flag(b)`（`Entity.get_flag`）：`a.team === b.team ? Ally : Enemy`，`a.hp <= 0` 时
// 再或上 `Dead`，最后或上 `a.type`（先 ToInt32 再或）。
//
// TS 里这是 `Entity` 上的方法。端口把它抽成自由函数，是因为 `BallController` 只有引用
// （自己的数据在 `CtrlEnv` 里、对方是个 `Value` 引用），拿不到 `Entity`；
// `Entity::get_flag` 也改成调这里，避免两份实现漂移。
inline double flag_between(const std::u16string& a_team, double a_hp, double a_type,
                           const std::u16string& b_team) {
  int32_t ret = a_team == b_team ? static_cast<int32_t>(HitFlag::Ally)
                                 : static_cast<int32_t>(HitFlag::Enemy);
  if (a_hp <= 0) ret |= static_cast<int32_t>(HitFlag::Dead);
  return static_cast<double>(ret | js_to_int32(a_type));
}

}
