// Mutation spec for the `entity_states` differential slice.
//
// Subjects: native/lfw/state/states.{h,cpp} + native/lfw/state/entity_states.cpp
// (the TS side reads the real `State` constructor and `ENTITY_STATES` map).
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `States::has` is not used by the ported game code yet; the harness exposes a
//    `run has` op so the lookup predicate is still covered.
//  * TS treats a numeric key and the string with the same digits as different
//    `Map` keys, and re-setting an existing key keeps the original position. The
//    port reproduces both, so `encode_key`'s prefix and `set`'s early return are
//    both observable (`run get s "0"` vs `run get n 0`, `run setdup` + `run tail`).
//  * `debugger;` in TS `set`/`add` only fires with a debugger attached: a duplicate
//    key is observable through the value/size, not through a side effect.
//  * `state_names.h` maps each class to its `constructor.name`; the names are part
//    of the observed output, and mutating a name is caught like a wiring mistake.
//  * `fallback`'s entity-type switch uses `===`, so `8` vs `"8"` differ; because the
//    second call goes through the string key both share, that path is covered too.
//  * The registry order comes from insertion; `set_in_range`/`set_all_of` sizes are
//    locked by `run size`, `run head`, `run tail` and the full `run dump`.

export default {
  subject: "entity_states",
  mutations: [
    {
      note: "string keys are encoded as numbers",
      file: "native/lfw/state/states.cpp",
      from: "  if (text != nullptr) return std::u16string(kStringPrefix) + *text;",
      to: "  if (text != nullptr) return std::u16string(kNumberPrefix) + *text;",
    },
    {
      note: "a missing key reports the first entry",
      file: "native/lfw/state/states.cpp",
      from: "  return it == _index.end() ? _entries.size() : it->second;",
      to: "  return it == _index.end() ? 0 : it->second;",
    },
    {
      note: "a present key reports as missing",
      file: "native/lfw/state/states.cpp",
      from: "std::size_t States::index_of(const Value& key) const {\n  const auto it = _index.find(encode_key(key));\n  return it == _index.end() ? _entries.size() : it->second;\n}",
      to: "std::size_t States::index_of(const Value& key) const {\n  const auto it = _index.find(encode_key(key));\n  return it == _index.end() ? 0 : _entries.size();\n}",
    },
    {
      note: "has always reports true",
      file: "native/lfw/state/states.cpp",
      from: "bool States::has(const Value& key) const { return index_of(key) != _entries.size(); }",
      to: "bool States::has(const Value& key) const { return true; }",
    },
    {
      note: "get never finds an entry",
      file: "native/lfw/state/states.cpp",
      from: "  if (i == _entries.size()) return nullptr;\n  return _entries[i].state.get();",
      to: "  return nullptr;",
    },
    {
      note: "get returns the last entry for any key",
      file: "native/lfw/state/states.cpp",
      from: "  return _entries[i].state.get();",
      to: "  return _entries[_entries.size() - 1].state.get();",
    },
    {
      note: "re-setting a key appends a second entry",
      file: "native/lfw/state/states.cpp",
      from: "    _entries[i].state = std::move(value);\n    _entries[i].class_name = std::move(class_name);\n    return;",
      to: "    _entries[i].state = std::move(value);\n    _entries[i].class_name = std::move(class_name);",
    },
    {
      note: "re-setting a key forgets the class name",
      file: "native/lfw/state/states.cpp",
      from: "    _entries[i].state = std::move(value);\n    _entries[i].class_name = std::move(class_name);",
      to: "    _entries[i].state = std::move(value);",
    },
    {
      note: "new entries are inserted at the front",
      file: "native/lfw/state/states.cpp",
      from: "  _entries.push_back(Entry{key, std::move(value), std::move(class_name)});",
      to: "  _entries.insert(_entries.begin(), Entry{key, std::move(value), std::move(class_name)});",
    },
    {
      note: "new entries are not indexed",
      file: "native/lfw/state/states.cpp",
      from: "  _index.emplace(encode_key(key), _entries.size());\n  _entries.push_back",
      to: "  _entries.push_back",
    },
    {
      note: "the fallback key uses a dash",
      file: "native/lfw/state/states.cpp",
      from: "  const std::u16string state_key = to_string(type) + u\"_\" + to_string(code);",
      to: "  const std::u16string state_key = to_string(type) + u\"-\" + to_string(code);",
    },
    {
      note: "the fallback key swaps type and code",
      file: "native/lfw/state/states.cpp",
      from: "  const std::u16string state_key = to_string(type) + u\"_\" + to_string(code);",
      to: "  const std::u16string state_key = to_string(code) + u\"_\" + to_string(type);",
    },
    {
      note: "the fallback never reuses a cached state",
      file: "native/lfw/state/states.cpp",
      from: "  if (State_Base* hit = get(key)) return *hit;",
      to: "  if (State_Base* hit = get(key)) { (void)hit; }",
    },
    {
      note: "the cached state is keyed by its own value",
      file: "native/lfw/state/states.cpp",
      from: "    return make<CharacterState_Base>(key, code);",
      to: "    return add<CharacterState_Base>(code);",
    },
    {
      note: "the fighter fallback builds a weapon state",
      file: "native/lfw/state/states.cpp",
      from: "    return make<CharacterState_Base>(key, code);",
      to: "    return make<WeaponState_Base>(key, code);",
    },
    {
      note: "the weapon fallback builds a fighter state",
      file: "native/lfw/state/states.cpp",
      from: "    return make<WeaponState_Base>(key, code);",
      to: "    return make<CharacterState_Base>(key, code);",
    },
    {
      note: "the ball fallback builds a bare state",
      file: "native/lfw/state/states.cpp",
      from: "    return make<BallState_Base>(key, code);",
      to: "    return make<State_Base>(key, code);",
    },
    {
      note: "the fallback always builds a bare state",
      file: "native/lfw/state/states.cpp",
      from: "  return make<State_Base>(key, code);",
      to: "  return make<State_Base>(key, Value(0.0));",
    },
    {
      note: "the fallback ignores the code when keying",
      file: "native/lfw/state/states.cpp",
      from: "  const Value key(state_key);",
      to: "  const Value key(Value(std::u16string(u\"\")));",
    },
    {
      note: "added states are keyed by a constant",
      file: "native/lfw/state/states.h",
      from: "    const Value key = value->state();\n    T& ref = *value;",
      to: "    const Value key = Value(0.0);\n    T& ref = *value;",
    },
    {
      note: "the range registers the upper bound as every state",
      file: "native/lfw/state/states.h",
      from: "  void set_in_range(double from, double to) {\n    for (double key = from; key <= to; ++key) {\n      make<T>(Value(key), Value(key));\n    }\n  }",
      to: "  void set_in_range(double from, double to) {\n    for (double key = from; key <= to; ++key) {\n      make<T>(Value(key), Value(to));\n    }\n  }",
    },
    {
      note: "the range excludes its upper bound",
      file: "native/lfw/state/states.h",
      from: "    for (double key = from; key <= to; ++key) {",
      to: "    for (double key = from; key < to; ++key) {",
    },
    {
      note: "the range steps by two",
      file: "native/lfw/state/states.h",
      from: "    for (double key = from; key <= to; ++key) {",
      to: "    for (double key = from; key <= to; key += 2) {",
    },
    {
      note: "the range starts one late",
      file: "native/lfw/state/states.h",
      from: "    for (double key = from; key <= to; ++key) {",
      to: "    for (double key = from + 1; key <= to; ++key) {",
    },
    {
      note: "the proxy list ignores its keys",
      file: "native/lfw/state/states.h",
      from: "    for (const Value& key : keys) {\n      make<T>(key, key);\n    }",
      to: "    for (const Value& key : keys) {\n      make<T>(key, Value(0.0));\n    }",
    },
    {
      note: "the proxy list registers a single key",
      file: "native/lfw/state/states.h",
      from: "  void set_all_of(const std::vector<Value>& keys) {\n    for (const Value& key : keys) {",
      to: "  void set_all_of(const std::vector<Value>& keys) {\n    for (const Value& key : std::vector<Value>{keys.empty() ? Value() : keys[0]}) {",
    },
    {
      note: "added states lose their class name",
      file: "native/lfw/state/states.h",
      from: "    set(key, std::move(value), state_name_u16<T>());\n    return ref;\n  }\n\n  // Explicit-key variant",
      to: "    set(key, std::move(value), state_name_u16<State_Base>());\n    return ref;\n  }\n\n  // Explicit-key variant",
    },
    {
      note: "made states lose their class name",
      file: "native/lfw/state/states.h",
      from: "    set(key, std::move(value), state_name_u16<T>());\n    return ref;\n  }\n\n  template <typename T>\n  void set_in_range",
      to: "    set(key, std::move(value), state_name_u16<State_Base>());\n    return ref;\n  }\n\n  template <typename T>\n  void set_in_range",
    },
    {
      note: "the transform range bound is wrong",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.set_in_range<State_TransformTo8XXX>(se(StateEnum::TransformTo_Min),\n                                             se(StateEnum::TransformTo_Max));",
      to: "  states.set_in_range<State_TransformTo8XXX>(se(StateEnum::TransformTo_Min) + 1.0,\n                                             se(StateEnum::TransformTo_Max));",
    },
    {
      note: "the transform range registers the wrong class",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.set_in_range<State_TransformTo8XXX>(",
      to: "  states.set_in_range<State_WeaponBroken>(",
    },
    {
      note: "the ball proxies use another class",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.set_all_of<StateBase_Proxy>({",
      to: "  states.set_all_of<State_Frozen>({",
    },
    {
      note: "one ball state is dropped",
      file: "native/lfw/state/entity_states.cpp",
      from: "      Value(se(StateEnum::Ball_3006)),\n",
      to: "",
    },
    {
      note: "one ball state is duplicated",
      file: "native/lfw/state/entity_states.cpp",
      from: "      Value(se(StateEnum::Ball_3005)),",
      to: "      Value(se(StateEnum::Ball_3006)),",
    },
    {
      note: "the broken weapon state is the catching state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<State_WeaponBroken>();",
      to: "  states.add<State_TransformToCatching>();",
    },
    {
      note: "the rebounding weapon state is the on-ground state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<WeaponState_Base>(Value(se(StateEnum::Weapon_Rebounding)));",
      to: "  states.add<WeaponState_OnGround>(Value(se(StateEnum::Weapon_Rebounding)));",
    },
    {
      note: "the in-the-sky weapon state is keyed as the light one",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<WeaponState_InTheSky>(Value(se(StateEnum::HeavyWeapon_InTheSky)));",
      to: "  states.add<WeaponState_InTheSky>(Value(se(StateEnum::Weapon_InTheSky)));",
    },
    {
      note: "the heavy throwing state reuses the just-on-ground key",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<WeaponState_Throwing>(Value(se(StateEnum::HeavyWeapon_JustOnGround)));",
      to: "  states.add<WeaponState_Throwing>(Value(se(StateEnum::HeavyWeapon_OnGround)));",
    },
    {
      note: "the entity base state is a character state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<State_Base>(Value(se(StateEnum::_Entity_Base)));",
      to: "  states.add<CharacterState_Base>(Value(se(StateEnum::_Entity_Base)));",
    },
    {
      note: "the character base state is the bare state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Base>(Value(se(StateEnum::_Character_Base)));",
      to: "  states.add<State_Base>(Value(se(StateEnum::_Character_Base)));",
    },
    {
      note: "the tired state reuses the injured key",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Injured>(Value(se(StateEnum::Tired)));",
      to: "  states.add<CharacterState_Injured>(Value(se(StateEnum::Injured)));",
    },
    {
      note: "the z-moveable key gets the bare state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Base>(Value(se(StateEnum::Z_Moveable)));",
      to: "  states.add<State_Base>(Value(se(StateEnum::Z_Moveable)));",
    },
    {
      note: "the nearest-enemy teleport is the farthest-ally one",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Teleport2NearestEnemy>();",
      to: "  states.add<CharacterState_Teleport2FarthestAlly>();",
    },
    {
      note: "the transform-to-louis state is the rowing state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_TransformToLouisEX>();",
      to: "  states.add<CharacterState_Rowing>();",
    },
    {
      note: "the rowing state is the drink state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Rowing>();",
      to: "  states.add<CharacterState_Drink>();",
    },
    {
      note: "the drink state is the jump state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Drink>();",
      to: "  states.add<CharacterState_Jump>();",
    },
    {
      note: "state 15 is a bare state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<State_15>();",
      to: "  states.add<State_Base>(Value(15.0));",
    },
    {
      note: "the land-goto state is not a proxy",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<StateBase_Proxy>(Value(se(StateEnum::LandGoto94)));",
      to: "  states.add<State_Base>(Value(se(StateEnum::LandGoto94)));",
    },
    {
      note: "the lying state is the fallen state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<CharacterState_Lying>();",
      to: "  states.add<CharacterState_Falling>();",
    },
    {
      note: "the burning state is the frozen state",
      file: "native/lfw/state/entity_states.cpp",
      from: "  states.add<State_Burning>();",
      to: "  states.add<State_Frozen>();",
    },
  ],
};
