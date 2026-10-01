#pragma once

#include <memory>

#include "lfw/core/value.h"

namespace lfw {

inline Value assign(const Value& output, const Value& item) {
  Value target = truthy(output) ? output : Value(std::make_shared<Object>());
  Object* t = as_object(target);
  if (t == nullptr) return target;
  const Object* src = as_object(item);
  if (src == nullptr) return target;
  for (const std::u16string& k : src->keys()) {
    const Value* v = src->get(k);
    if (v != nullptr) t->set(k, *v);
  }
  return target;
}

}
