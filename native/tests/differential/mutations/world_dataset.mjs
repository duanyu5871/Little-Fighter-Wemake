// Mutation spec for the `world_dataset` differential slice.
//
// Subject: native/lfw/world_dataset.{h,cpp} (the TS side drives the real
// `src/LFW/WorldDataset.ts` class).
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `make_private_properties` publishes each trapped field's old value through a
//    `_$_<key>` backup property and looks per-field hooks up as `on_<key>_change`,
//    and the change callback itself is an own property. The port models the field
//    set and the two callback slots as data, so the harness normalizes `Object.keys`
//    (drops `on_*` and `_$_*`) on the TS side instead of the port faking JS property
//    internals. That is a harness convention, not a behavioral difference.
//  * Every one of the 103 defaults is present in `world_dataset_fields()`, so a
//    mutation that drops the table membership test in the constructor is equivalent
//    by construction and is not listed (the membership is still exercised through
//    `tracked`, which is exactly `!_pure && key in _tracked`).
//  * `keys()` (the full own-property order) is only used by the harness through
//    `run keys`, which reports the enumerable subset; the two are the same list for
//    a `pure` dataset, and for a non-pure dataset the accessor-installed names are
//    hidden, so `enumerable_keys` is what the trace pins.
//  * The mutation runner rebuilds and compares whole traces, so a wrong literal, a
//    dropped field, a flipped flag or a missing notification all surface as drift.

export default {
  subject: "world_dataset",
  mutations: [
    {
      note: "the lazily created default instance is not shared",
      file: "native/lfw/world_dataset.cpp",
      from:
        "WorldDataset& WorldDataset::default_instance() {\n  static WorldDataset instance;\n  return instance;\n}",
      to:
        "WorldDataset& WorldDataset::default_instance() {\n  static WorldDataset* instance = nullptr;\n  instance = new WorldDataset(false);\n  return *instance;\n}",
    },
    {
      note: "the default instance is built as a pure dataset",
      file: "native/lfw/world_dataset.cpp",
      from: "  static WorldDataset instance;\n  return instance;",
      to: "  static WorldDataset instance(true);\n  return instance;",
    },
    {
      note: "the pure flag is inverted in the constructor",
      file: "native/lfw/world_dataset.cpp",
      from: "  install_defaults();\n  if (pure) return;",
      to: "  install_defaults();\n  if (!pure) return;",
    },
    {
      note: "a pure dataset also records the marker property",
      file: "native/lfw/world_dataset.cpp",
      from: "  install_defaults();\n  if (pure) return;",
      to:
        "  install_defaults();\n  if (pure) {\n    _keys.emplace_back(u\"__is_world_dataset__\");\n    return;\n  }",
    },
    {
      note: "the defaults are installed twice",
      file: "native/lfw/world_dataset.cpp",
      from: "  install_defaults();\n  if (pure) return;",
      to: "  install_defaults();\n  install_defaults();\n  if (pure) return;",
    },
    {
      note: "the defaults are never installed",
      file: "native/lfw/world_dataset.cpp",
      from: "  install_defaults();\n  if (pure) return;",
      to: "  if (pure) return;",
    },
    {
      note: "the tracked set is built from the wrong snapshot",
      file: "native/lfw/world_dataset.cpp",
      from: "    for (const std::u16string& key : _keys) {\n      if (fields->has(key)) _tracked.insert(key);\n    }",
      to:
        "    for (const std::u16string& key : _keys) {\n      if (fields->has(key)) _tracked.insert(u\"__is_world_dataset__\");\n    }",
    },
    {
      note: "the marker property is tracked",
      file: "native/lfw/world_dataset.cpp",
      from: "  _keys.emplace_back(u\"__is_world_dataset__\");\n  _values.emplace(u\"__is_world_dataset__\", Value(true));",
      to:
        "  _keys.emplace_back(u\"__is_world_dataset__\");\n  _values.emplace(u\"__is_world_dataset__\", Value(true));\n  _tracked.insert(u\"__is_world_dataset__\");",
    },
    {
      note: "the marker value is false",
      file: "native/lfw/world_dataset.cpp",
      from: "  _values.emplace(u\"__is_world_dataset__\", Value(true));",
      to: "  _values.emplace(u\"__is_world_dataset__\", Value(false));",
    },
    {
      note: "the marker value is stored without its key",
      file: "native/lfw/world_dataset.cpp",
      from: "  _keys.emplace_back(u\"__is_world_dataset__\");\n  _values.emplace(u\"__is_world_dataset__\", Value(true));",
      to: "  _values.emplace(u\"__is_world_dataset__\", Value(true));",
    },
    {
      note: "installed defaults are not recorded in the key order",
      file: "native/lfw/world_dataset.cpp",
      from: "void WorldDataset::add(const char16_t* key, double value) {\n  _keys.emplace_back(key);",
      to: "void WorldDataset::add(const char16_t* key, double value) {",
    },
    {
      note: "installed defaults all read as zero",
      file: "native/lfw/world_dataset.cpp",
      from: "  _values.emplace(std::u16string(key), Value(value));",
      to: "  _values.emplace(std::u16string(key), Value(0.0));",
    },
    {
      note: "a default literal is off (jump_height)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"jump_height", -16.299999);',
      to: '  add(u"jump_height", -16.3);',
    },
    {
      note: "a default literal is off (screen_w)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"screen_w", 794);',
      to: '  add(u"screen_w", 793);',
    },
    {
      note: "a default literal is off (screen_h)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"screen_h", 450);',
      to: '  add(u"screen_h", 451);',
    },
    {
      note: "a default literal is off (sync_render)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"sync_render", 3);',
      to: '  add(u"sync_render", 4);',
    },
    {
      note: "a default literal is off (difficulty)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"difficulty", 3);',
      to: '  add(u"difficulty", 4);',
    },
    {
      note: "a default literal is off (hp_max)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"hp_max", 500);',
      to: '  add(u"hp_max", 400);',
    },
    {
      note: "a default literal is negated (gravity)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"gravity", 0.4375);',
      to: '  add(u"gravity", -0.4375);',
    },
    {
      note: "a default literal is off by a fraction (gravity_d)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"gravity_d", 0.4375);',
      to: '  add(u"gravity_d", 0.4376);',
    },
    {
      note: "a default field is dropped (outline_enabled)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"outline_enabled", 1);\n',
      to: "",
    },
    {
      note: "a default field is dropped (UPS)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"UPS", 60);\n',
      to: "",
    },
    {
      note: "a default field is duplicated (itr_fall)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"itr_fall", 40);\n',
      to: '  add(u"itr_fall", 40);\n  add(u"itr_fall", 40);\n',
    },
    {
      note: "two defaults are swapped in the declaration order",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"itr_fall", 40);\n  add(u"itr_shaking", 8);',
      to: '  add(u"itr_shaking", 8);\n  add(u"itr_fall", 40);',
    },
    {
      note: "a default field is dropped (itr_fall)",
      file: "native/lfw/world_dataset.cpp",
      from: '  add(u"itr_fall", 40);\n',
      to: "",
    },
    {
      note: "a default field is duplicated at the end",
      file: "native/lfw/world_dataset.cpp",
      from: "  add(u\"LF2_NET\", 0);\n}",
      to: "  add(u\"LF2_NET\", 0);\n  add(u\"itr_fall\", 40);\n}",
    },
    {
      note: "has() answers from the tracked set",
      file: "native/lfw/world_dataset.cpp",
      from: "bool WorldDataset::has(const std::u16string& key) const {\n  return _values.find(key) != _values.end();",
      to: "bool WorldDataset::has(const std::u16string& key) const {\n  return tracked(key);",
    },
    {
      note: "has() reports the marker as missing",
      file: "native/lfw/world_dataset.cpp",
      from: "  return _values.find(key) != _values.end();",
      to: "  return key != u\"__is_world_dataset__\" && _values.find(key) != _values.end();",
    },
    {
      note: "tracked() also sees pure datasets",
      file: "native/lfw/world_dataset.cpp",
      from: "  return !_pure && _tracked.find(key) != _tracked.end();",
      to: "  return _pure || _tracked.find(key) != _tracked.end();",
    },
    {
      note: "tracked() consults the key order instead of the tracked set",
      file: "native/lfw/world_dataset.cpp",
      from: "  return !_pure && _tracked.find(key) != _tracked.end();",
      to: "  return !_pure && std::find(_keys.begin(), _keys.end(), key) != _keys.end();",
    },
    {
      note: "the enumerable key list keeps trapped fields",
      file: "native/lfw/world_dataset.cpp",
      from: "  for (const std::u16string& key : _keys) {\n    if (tracked(key)) continue;\n    out.push_back(key);\n  }",
      to: "  for (const std::u16string& key : _keys) {\n    out.push_back(key);\n  }",
    },
    {
      note: "the enumerable key list hides every key of a pure dataset",
      file: "native/lfw/world_dataset.cpp",
      from: "  for (const std::u16string& key : _keys) {\n    if (tracked(key)) continue;\n    out.push_back(key);\n  }",
      to: "  for (const std::u16string& key : _keys) {\n    if (tracked(key) || _pure) continue;\n    out.push_back(key);\n  }",
    },
    {
      note: "the enumerable key list is reversed",
      file: "native/lfw/world_dataset.cpp",
      from: "  for (const std::u16string& key : _keys) {\n    if (tracked(key)) continue;\n    out.push_back(key);\n  }",
      to:
        "  for (auto it = _keys.rbegin(); it != _keys.rend(); ++it) {\n    if (tracked(*it)) continue;\n    out.push_back(*it);\n  }",
    },
    {
      note: "a missing key reads as zero",
      file: "native/lfw/world_dataset.cpp",
      from: "  return it == _values.end() ? Value() : it->second;",
      to: "  return it == _values.end() ? Value(0.0) : it->second;",
    },
    {
      note: "an existing key always reads as undefined",
      file: "native/lfw/world_dataset.cpp",
      from: "  return it == _values.end() ? Value() : it->second;",
      to: "  return Value();",
    },
    {
      note: "a write to a brand new key is ignored",
      file: "native/lfw/world_dataset.cpp",
      from: "    _keys.push_back(key);\n    _values.emplace(key, std::move(v));\n    return;",
      to: "    return;",
    },
    {
      note: "a brand new key joins the tracked set",
      file: "native/lfw/world_dataset.cpp",
      from: "    _keys.push_back(key);\n    _values.emplace(key, std::move(v));\n    return;",
      to: "    _keys.push_back(key);\n    _values.emplace(key, std::move(v));\n    _tracked.insert(key);\n    return;",
    },
    {
      note: "a brand new key is stored as undefined",
      file: "native/lfw/world_dataset.cpp",
      from: "    _keys.push_back(key);\n    _values.emplace(key, std::move(v));\n    return;",
      to: "    _keys.push_back(key);\n    _values.emplace(key, Value());\n    return;",
    },
    {
      note: "an unchanged write still notifies",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (strict_equals(it->second, v)) return;",
      to: "  if (strict_equals(it->second, v) && false) return;",
    },
    {
      note: "the change notification reports the new value as the previous one",
      file: "native/lfw/world_dataset.cpp",
      from: "  const Value prev = it->second;",
      to: "  const Value prev = v;",
    },
    {
      note: "untracked keys notify as well",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (!tracked(key)) return;",
      to: "  if (false) return;",
    },
    {
      note: "the field hook runs after the change callback",
      file: "native/lfw/world_dataset.cpp",
      from:
        "  const auto hook = _hooks.find(key);\n  if (hook != _hooks.end() && hook->second) hook->second(it->second, prev);\n  if (on_dataset_change) on_dataset_change(key, it->second, prev);",
      to:
        "  if (on_dataset_change) on_dataset_change(key, it->second, prev);\n  const auto hook = _hooks.find(key);\n  if (hook != _hooks.end() && hook->second) hook->second(it->second, prev);",
    },
    {
      note: "the field hook is looked up under another key",
      file: "native/lfw/world_dataset.cpp",
      from: "  const auto hook = _hooks.find(key);",
      to: "  const auto hook = _hooks.find(u\"gravity\");",
    },
    {
      note: "the field hook is never called",
      file: "native/lfw/world_dataset.cpp",
      from: "  const auto hook = _hooks.find(key);\n  if (hook != _hooks.end() && hook->second) hook->second(it->second, prev);",
      to: "  (void)_hooks;",
    },
    {
      note: "the change callback is never called",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (on_dataset_change) on_dataset_change(key, it->second, prev);",
      to: "  (void)on_dataset_change;",
    },
    {
      note: "the change callback receives an empty key",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (on_dataset_change) on_dataset_change(key, it->second, prev);",
      to: "  if (on_dataset_change) on_dataset_change(u\"\", it->second, prev);",
    },
    {
      note: "the change callback receives the arguments in the other order",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (on_dataset_change) on_dataset_change(key, it->second, prev);",
      to: "  if (on_dataset_change) on_dataset_change(key, prev, it->second);",
    },
    {
      note: "the registered field hook is forgotten",
      file: "native/lfw/world_dataset.cpp",
      from: "  _hooks[key] = std::move(hook);",
      to: "  _hooks[key];",
    },
    {
      note: "the registered field hook is filed under an empty key",
      file: "native/lfw/world_dataset.cpp",
      from: "  _hooks[key] = std::move(hook);",
      to: "  _hooks[u\"\"] = std::move(hook);",
    },
    {
      note: "the dumped table is not sorted",
      file: "native/lfw/world_dataset.cpp",
      from: "  std::sort(keys.begin(), keys.end());\n",
      to: "",
    },
    {
      note: "the dumped table is sorted in reverse",
      file: "native/lfw/world_dataset.cpp",
      from: "  std::sort(keys.begin(), keys.end());",
      to: "  std::sort(keys.begin(), keys.end(), std::greater<std::u16string>());",
    },
    {
      note: "the dump enumerates the instance keys instead of the field table",
      file: "native/lfw/world_dataset.cpp",
      from: "  if (fields != nullptr) keys = fields->keys();",
      to: "  if (fields != nullptr) keys = _keys;",
    },
    {
      note: "the dump skips the second half of the table",
      file: "native/lfw/world_dataset.cpp",
      from: "  std::sort(keys.begin(), keys.end());",
      to: "  std::sort(keys.begin(), keys.end());\n  keys.resize(keys.size() / 2);",
    },
    {
      note: "the dumped values are read from the wrong key",
      file: "native/lfw/world_dataset.cpp",
      from: "    ret.set(key, get(key));",
      to: "    ret.set(key, Value());",
    },
    {
      note: "a tracked field write is not stored",
      file: "native/lfw/world_dataset.cpp",
      from: "  it->second = std::move(v);\n  if (!tracked(key)) return;",
      to: "  it->second = std::move(v);\n  if (!tracked(key)) return;\n  it->second = prev;",
    },
  ],
};
