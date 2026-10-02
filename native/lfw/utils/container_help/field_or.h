#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {

inline Value field_or(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

inline Value field_or(const Object& o, const char16_t* key) {
  const Value* p = o.get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

}
