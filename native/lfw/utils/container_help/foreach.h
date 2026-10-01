#pragma once

#include <functional>
#include <variant>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

inline void foreach(const Value& any, const std::function<void(Value&, size_t)>& fn) {
  if (std::holds_alternative<std::monostate>(any) || std::holds_alternative<NullTag>(any)) return;
  Value copy = any;
  Array* a = as_array(copy);
  if (a == nullptr) return;
  for (size_t i = 0; i < a->size(); ++i) fn(a->at(i), i);
}

}
