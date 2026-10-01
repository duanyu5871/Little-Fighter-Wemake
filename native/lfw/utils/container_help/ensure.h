#pragma once

#include <optional>
#include <vector>

namespace lfw {

template <typename T>
std::vector<T> ensure(std::optional<std::vector<T>>& output, const std::vector<T>& items) {
  if (!output.has_value()) return items;
  output->insert(output->end(), items.begin(), items.end());
  return *output;
}

}
