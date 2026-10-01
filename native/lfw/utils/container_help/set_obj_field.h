#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"

namespace lfw {

inline Value& set_obj_field(Value& target, const std::u16string& key, const Value& value) {
  Object* o = as_object(target);
  if (o == nullptr) {
    target = Value(std::make_shared<Object>());
    o = as_object(target);
  }
  if (o != nullptr) o->set(key, value);
  return target;
}

}
