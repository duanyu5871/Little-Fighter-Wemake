#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

// TS 的 `find`（`utils/container_help/find.ts`）在可迭代对象上扫完元素之后**还会**跑一遍
// `for (const k in p0) if (p1([k, p0[k]])) return [k, p0[k]]`（两个循环之间没有 `else`）。
// 对数组来说这一步会把 `["0", v0]`… 交给同一个谓词；谓词按字段读时
// （`o => o.interval_id === x`）从 pair 上读到 `undefined` ⇒ 「拿 `undefined` 去比」的谓词
// 会在元素一个都不匹配时拿到真值。这里把该回落显式建模出来（元素扫完没命中就把 pair
// 视图再喂同一个谓词），否则 `set_frame` 的 opoint 压实这类调用点会与 TS 分叉。
inline std::optional<Value> find_array(const Array& a,
                                       const std::function<bool(const Value&)>& pred) {
  if (a.empty()) return std::nullopt;
  for (size_t i = 0; i < a.size(); ++i) {
    if (pred(a.at(i))) return a.at(i);
  }
  for (size_t i = 0; i < a.size(); ++i) {
    std::vector<Value> items;
    items.push_back(Value(number_to_string(static_cast<double>(i))));
    items.push_back(a.at(i));
    const Value pair(std::make_shared<Array>(std::move(items)));
    if (pred(pair)) return pair;
  }
  return std::nullopt;
}

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
