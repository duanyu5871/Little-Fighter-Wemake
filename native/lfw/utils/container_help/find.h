#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

inline size_t find_value_index(const Array* a, const std::function<bool(const Value&)>& pred) {
  if (a == nullptr) return SIZE_MAX;
  for (size_t i = 0; i < a->size(); ++i) {
    if (pred(a->at(i))) return i;
  }
  return SIZE_MAX;
}

template <typename C, typename P>
std::optional<typename C::value_type> find(const C& set, P p) {
  for (const auto& v : set) {
    if (p(v)) return v;
  }
  return std::nullopt;
}

template <typename C, typename P>
std::optional<typename C::value_type> find_last(const C& set, P p) {
  const std::vector<typename C::value_type> arr(set.begin(), set.end());
  for (size_t i = arr.size(); i > 0; --i) {
    if (p(arr[i - 1])) return arr[i - 1];
  }
  return std::nullopt;
}

template <typename T, typename F = std::equal_to<T>>
std::vector<T> intersection(const std::vector<T>& a, const std::vector<T>& b, F fn = F{}) {
  std::vector<T> ret;
  for (const auto& c1 : a) {
    for (const auto& c2 : b) {
      if (fn(c1, c2)) {
        ret.push_back(c1);
        break;
      }
    }
  }
  return ret;
}

}
