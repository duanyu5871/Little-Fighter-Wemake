#pragma once

#include <memory>

#include "lfw/core/value.h"

namespace lfw {

// `a === b` / `a !== b` —— TS 比的是**对象同一性**。端口的 `Value` 里放的是
// `shared_ptr`，同一个记录的所有副本共享同一个指针 ⇒ 同一性就是指针相等；
// 两个字段相等的不同对象在这里**不**相等，跟 TS 一致。
//
// （`entity.cpp` 里原本有一份同名的局部实现，本刀把它抽到这里，好让
// `controller/ball_controller.cpp` 的 `this.frame != frame` 用同一份。）
inline bool same_ref(const Value& a, const Value& b) {
  const auto* pa = std::get_if<std::shared_ptr<Object>>(&a);
  const auto* pb = std::get_if<std::shared_ptr<Object>>(&b);
  if (pa != nullptr && pb != nullptr) return *pa == *pb;
  const auto* aa = std::get_if<std::shared_ptr<Array>>(&a);
  const auto* ab = std::get_if<std::shared_ptr<Array>>(&b);
  if (aa != nullptr && ab != nullptr) return *aa == *ab;
  return false;
}

}
