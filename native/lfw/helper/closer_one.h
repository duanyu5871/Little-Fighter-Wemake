#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace helper {

// `closer_one(s, t1, t2)`：取离 `s` 更近的那个（曼哈顿 xz），都没有就给空值。
// 端口沿用 `manhattan_xz` 的约定：三个参数都是 `Value` 引用（`helper` 不认识 `Entity`），
// `t1` / `t2` 传空值就表示 TS 的 `undefined`。
Value closer_one(const Value& s, const Value& t1, const Value& t2);

}
}
