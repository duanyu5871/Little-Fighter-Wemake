#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/state/state_base.h"
#include "lfw/state/state_names.h"

namespace lfw {
namespace state {

// Registry of states, mirroring `src/LFW/state/States.ts`.
//
// TS keeps a `Map<number | string, State_Base>`; JS keys distinguish `1` from
// `"1"`, so entries keep their original `Value` key (in insertion order, which
// `Map` preserves even when a key is re-set) and lookups go through an encoded
// string key that carries the type.
class States {
 public:
  struct Entry {
    Value key;
    std::unique_ptr<State_Base> state;
    std::u16string class_name;
  };

  State_Base* get(const Value& key) const;
  void set(const Value& key, std::unique_ptr<State_Base> value,
           std::u16string class_name);
  bool has(const Value& key) const;
  std::size_t size() const { return _entries.size(); }
  const std::vector<Entry>& entries() const { return _entries; }

  // Mirrors TS `States.add(...values)`: the key is the state's own value.
  template <typename T, typename... Args>
  T& add(Args&&... args) {
    auto value = std::make_unique<T>(std::forward<Args>(args)...);
    const Value key = value->state();
    T& ref = *value;
    set(key, std::move(value), state_name_u16<T>());
    return ref;
  }

  // Explicit-key variant, used by `set_in_range` / `set_all_of` / `fallback`.
  template <typename T, typename... Args>
  T& make(const Value& key, Args&&... args) {
    auto value = std::make_unique<T>(std::forward<Args>(args)...);
    T& ref = *value;
    set(key, std::move(value), state_name_u16<T>());
    return ref;
  }

  template <typename T>
  void set_in_range(double from, double to) {
    for (double key = from; key <= to; ++key) {
      make<T>(Value(key), Value(key));
    }
  }

  template <typename T>
  void set_all_of(const std::vector<Value>& keys) {
    for (const Value& key : keys) {
      make<T>(key, key);
    }
  }

  State_Base& fallback(const Value& type, const Value& code);

 private:
  static std::u16string encode_key(const Value& key);
  std::size_t index_of(const Value& key) const;

  std::vector<Entry> _entries;
  std::map<std::u16string, std::size_t> _index;
};

}
}
