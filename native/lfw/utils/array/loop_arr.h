#pragma once

#include <vector>

namespace lfw {

template <typename Fn>
void loop_arr(const std::vector<double>& list, Fn fn) {
  for (size_t i = 0; i < list.size(); ++i) fn(list[i], i, list);
}

template <typename Fn>
void loop_arr(double item, Fn fn) {
  fn(item, 0, std::vector<double>{item});
}

}
