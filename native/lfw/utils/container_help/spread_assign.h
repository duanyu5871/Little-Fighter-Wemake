#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

inline Value spread_assign(const Value& a, const Value& b) {
  Object out;
  const Object* ao = as_object(a);
  if (ao != nullptr) {
    const std::vector<std::u16string> ks = ao->keys();
    for (const std::u16string& k : ks) out.set(k, *ao->get(k));
  }
  const Object* bo = as_object(b);
  if (bo != nullptr) {
    const std::vector<std::u16string> ks = bo->keys();
    for (const std::u16string& k : ks) out.set(k, *bo->get(k));
  }
  return Value(std::make_shared<Object>(out));
}

}
