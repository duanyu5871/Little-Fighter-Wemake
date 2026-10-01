#pragma once

#include <cmath>
#include <optional>
#include <vector>

namespace lfw {

template <typename T>
std::optional<T> loop_offset(const std::vector<T>& list, const T& current, double offset) {
  const size_t len = list.size();
  if (len == 0) return std::nullopt;

  double idx = -1.0;
  for (size_t i = 0; i < len; ++i) {
    if (list[i] == current) {
      idx = static_cast<double>(i);
      break;
    }
  }

  offset = std::fmod(offset, static_cast<double>(len));
  if (offset > 0.0) {
    idx = std::fmod(idx + offset, static_cast<double>(len));
  } else {
    idx = std::fmod(static_cast<double>(len) + idx + offset, static_cast<double>(len));
  }

  if (!(idx >= 0.0)) return std::nullopt;
  if (idx != std::floor(idx)) return std::nullopt;
  if (idx >= static_cast<double>(len)) return std::nullopt;
  return list[static_cast<size_t>(idx)];
}

}
