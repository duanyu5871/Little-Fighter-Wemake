#pragma once

#include <vector>

namespace lfw {

template <typename C, typename P>
auto filter(const C& set, P p) -> std::vector<typename C::value_type> {
  std::vector<typename C::value_type> ret;
  for (const auto& i : set) {
    if (p(i)) ret.push_back(i);
  }
  return ret;
}

}
