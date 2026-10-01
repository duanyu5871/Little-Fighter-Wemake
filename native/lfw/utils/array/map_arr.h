#pragma once

#include <optional>
#include <vector>

namespace lfw {

template <typename Fn>
auto map_arr(const std::optional<std::vector<double>>& list, Fn fn) {
  using R = decltype(fn(0.0, 0.0, std::vector<double>{}));
  std::vector<R> ret;
  if (!list.has_value()) return ret;
  const std::vector<double>& v = *list;
  for (size_t i = 0; i < v.size(); ++i) ret.push_back(fn(v[i], i, v));
  return ret;
}

template <typename Fn>
auto map_arr(double list, Fn fn) {
  using R = decltype(fn(0.0, 0.0, std::vector<double>{}));
  std::vector<R> ret;
  ret.push_back(fn(list, 0, std::vector<double>{list}));
  return ret;
}

}
