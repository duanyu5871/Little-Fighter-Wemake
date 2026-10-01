#pragma once

#include <vector>

#include "lfw/utils/container_help/nested_map.h"

namespace lfw {

template <typename K1, typename K2, typename V>
class NestedMultiMap {
 public:
  void add(const K1& k1, const K2& k2, const V& value) {
    _map.ref(k1, k2).push_back(value);
  }

  std::optional<V> first(const K1& k1, const K2& k2) const {
    const std::optional<std::vector<V>> ret = _map.get(k1, k2);
    if (!ret.has_value() || ret->empty()) return std::nullopt;
    return (*ret)[0];
  }

  bool has(const K1& k1, const K2& k2) const { return _map.has(k1, k2); }

  std::vector<V> collect(const K1& k1, const K2& k2, std::vector<V> out = {}) const {
    const std::optional<std::vector<V>> ret = _map.get(k1, k2);
    if (!ret.has_value()) return out;
    for (const auto& v : *ret) out.push_back(v);
    return out;
  }

  bool remove(const K1& k1, const K2& k2) { return _map.remove(k1, k2); }

  void clear() { _map.clear(); }

 private:
  NestedMap<K1, K2, std::vector<V>> _map;
};

}
