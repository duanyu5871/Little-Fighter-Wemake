#pragma once

#include <vector>

namespace lfw {

template <typename Fn>
auto make_arr(double size, Fn fn) {
  using R = decltype(fn(0.0));
  std::vector<R> ret;
  for (double i = 0.0; i < size; i += 1.0) ret.push_back(fn(i));
  return ret;
}

}
