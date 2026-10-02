#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {

inline Value n(double v) { return Value(v); }
inline Value s(const char16_t* v) { return Value(std::u16string(v)); }

template <typename E>
inline Value en(E v) {
  return Value(static_cast<double>(v));
}

inline Value make_obj(std::initializer_list<std::pair<const char16_t*, Value>> kv) {
  Object o;
  for (const std::pair<const char16_t*, Value>& p : kv) o.set(std::u16string(p.first), p.second);
  return Value(std::make_shared<Object>(o));
}

inline Value make_arr(std::initializer_list<Value> items) {
  Array a;
  for (const Value& v : items) a.push_back(v);
  return Value(std::make_shared<Array>(a));
}

inline Object* frame_obj(Value& frame) { return as_object(frame); }

inline bool same_str(const Value& v, const char16_t* str) {
  return strict_equals(v, Value(std::u16string(str)));
}

}

}
