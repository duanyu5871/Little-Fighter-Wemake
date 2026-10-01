#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

template <typename T>
std::vector<T> ensure(std::optional<std::vector<T>>& output, const std::vector<T>& items) {
  if (!output.has_value()) return items;
  output->insert(output->end(), items.begin(), items.end());
  return *output;
}

inline Value ensure(Value& output, const std::vector<Value>& items) {
  if (Array* a = as_array(output)) {
    for (const Value& v : items) a->push_back(v);
    return output;
  }
  Array fresh;
  for (const Value& v : items) fresh.push_back(v);
  output = Value(std::make_shared<Array>(fresh));
  return output;
}

inline Value ensure(Value& output, const Value& item) { return ensure(output, std::vector<Value>{item}); }

}
