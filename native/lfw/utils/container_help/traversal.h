#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

inline void traversal(Value& obj, const std::function<void(const std::u16string&, Value&)>& fn) {
  if (Array* a = as_array(obj)) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = i < a->size() ? a->at(i) : Value();
      fn(number_to_string(static_cast<double>(i)), item);
    }
    return;
  }
  Object* o = as_object(obj);
  if (o == nullptr) return;
  const std::vector<std::u16string> keys = o->keys();
  for (const std::u16string& k : keys) {
    const Value* found = o->get(k);
    Value item = found != nullptr ? *found : Value();
    fn(k, item);
  }
}

}

