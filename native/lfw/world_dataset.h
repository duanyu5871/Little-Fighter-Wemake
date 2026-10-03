#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// Mirrors `src/LFW/WorldDataset.ts`: a flat table of tuning values with change
// notification. TS installs a getter/setter pair (`make_private_properties`) for
// every own field that exists in `world_dataset_fields`, so assigning such a field
// fires `on_<key>_change` (when the host defined it) and then `on_dataset_change`;
// `new WorldDataset(true)` skips that install and stays a plain record.
class WorldDataset {
 public:
  using FieldHook = std::function<void(const Value&, const Value&)>;
  using ChangeCallback =
      std::function<void(const std::u16string&, const Value&, const Value&)>;

  static WorldDataset& default_instance();

  explicit WorldDataset(bool pure = false);

  bool has(const std::u16string& key) const;
  bool tracked(const std::u16string& key) const;
  Value get(const std::u16string& key) const;
  void set(const std::u16string& key, Value v);
  // Every own property, in insertion order (`Object.getOwnPropertyNames`).
  const std::vector<std::u16string>& keys() const { return _keys; }
  // `Object.keys()`: the accessor-installed fields are non-enumerable in TS, so a
  // non-pure dataset only lists the keys that never got an accessor.
  std::vector<std::u16string> enumerable_keys() const;

  void set_field_hook(const std::u16string& key, FieldHook hook);
  ChangeCallback on_dataset_change;

  Value dump_dataset() const;

 private:
  void add(const char16_t* key, double value);
  void install_defaults();

  bool _pure = false;
  std::vector<std::u16string> _keys;
  std::map<std::u16string, Value> _values;
  std::set<std::u16string> _tracked;
  std::map<std::u16string, FieldHook> _hooks;
};

}
