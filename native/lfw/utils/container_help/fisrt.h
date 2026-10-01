#pragma once

#include <optional>
#include <type_traits>

namespace lfw {

template <typename C>
std::optional<typename C::value_type> fisrt(const C& iterable) {
  for (const auto& item : iterable) return item;
  return std::nullopt;
}

template <typename C, typename P>
auto fisrt(const C& iterable, P p) -> std::invoke_result_t<P, typename C::value_type> {
  using R = std::invoke_result_t<P, typename C::value_type>;
  for (const auto& item : iterable) {
    R r = p(item);
    if (r.has_value()) return r;
  }
  return R{};
}

template <typename C>
std::optional<typename C::value_type> last(const C& iterable) {
  std::optional<typename C::value_type> ret;
  for (const auto& item : iterable) ret = item;
  return ret;
}

template <typename C, typename P>
auto last(const C& iterable, P p) -> std::invoke_result_t<P, typename C::value_type> {
  using R = std::invoke_result_t<P, typename C::value_type>;
  const std::vector<typename C::value_type> arr(iterable.begin(), iterable.end());
  for (size_t i = arr.size(); i > 0; --i) {
    R r = p(arr[i - 1]);
    if (r.has_value()) return r;
  }
  return R{};
}

}
