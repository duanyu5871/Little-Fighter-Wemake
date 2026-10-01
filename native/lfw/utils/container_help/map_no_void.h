#pragma once

#include <optional>
#include <type_traits>
#include <vector>

namespace lfw {

template <typename C, typename P>
auto map_no_void(const C& iterable, P p) {
  using R = typename std::invoke_result_t<P, typename C::value_type>::value_type;
  std::vector<R> ret;
  for (const auto& item : iterable) {
    auto r = p(item);
    if (r.has_value()) ret.push_back(*r);
  }
  return ret;
}

}
