#pragma once

#include <map>
#include <optional>

#include "lfw/base/graves.h"

namespace lfw {

template <typename K1, typename K2, typename V>
class NestedMap {
 public:
  using Inner = std::map<K2, V>;

  std::optional<V> get(const K1& k1, const K2& k2) const {
    const auto it = _map.find(k1);
    if (it == _map.end()) return std::nullopt;
    const auto it2 = it->second.find(k2);
    if (it2 == it->second.end()) return std::nullopt;
    return it2->second;
  }

  bool has(const K1& k1, const K2& k2) const {
    const auto it = _map.find(k1);
    return it != _map.end() && it->second.find(k2) != it->second.end();
  }

  V& ref(const K1& k1, const K2& k2) {
    auto it = _map.find(k1);
    if (it == _map.end()) {
      const std::optional<Inner> pooled = _graves.take();
      it = _map.emplace(k1, pooled.has_value() ? *pooled : Inner{}).first;
    }
    return it->second[k2];
  }

  void set(const K1& k1, const K2& k2, const V& value) { ref(k1, k2) = value; }

  bool remove(const K1& k1, const K2& k2) {
    const auto it = _map.find(k1);
    return it != _map.end() && it->second.erase(k2) != 0;
  }

  void clear() {
    if (_map.empty()) return;
    for (auto& kv : _map) {
      kv.second.clear();
      _graves.add(kv.second);
    }
    _map.clear();
  }

 private:
  std::map<K1, Inner> _map;
  Graves<Inner> _graves;
};

}
