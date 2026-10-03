/**
 * Mutation spec for `native/lfw/buff/buff_magic_flute.{h,cpp}`.
 *
 * The two TS classes carry byte-identical `on_tick` / `on_update` bodies apart from
 * two constants, so the port factors those bodies into the file-local helpers
 * `apply_flute_tick` / `apply_flute_update`; the class methods only pick the
 * constants. Every mutation below therefore anchors on the helper text or on the
 * constant that selects the behaviour.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `static constexpr int KIND` is unobservable in this unit: neither class reads it
 *    (no `set_mark` / `del_mark`, no `grant_buff` call). It belongs to whoever calls
 *    `grant_buff(..., "MagicFlute", ...)`.
 * 2. `if (victim == nullptr) return;` in `on_tick` / `on_update` cannot be inverted
 *    (null dereference) and can never be taken: the harness resolves every victim id
 *    before the buff is built.
 * 3. The trailing `return;` that ends the Fighter arm of `apply_flute_update` is
 *    unobservable: falling through to the Weapon test only adds
 *    `strict_equals(type, Weapon)`, which is false whenever the Fighter test was true.
 * 4. `index_by`'s array branch and `index_0`'s object branch are both dead:
 *    `index_by` is only ever called with `data.indexes.falling` (an object) and
 *    `index_0` only with `falling["-1"]` / `in_the_skys` (arrays). A `falling` that is
 *    itself an array makes the TS original throw (`falling[-1]` is `undefined`, then
 *    `undefined[0]` is a TypeError), so the case cannot reach it either way.
 * 5. `victim->set_velocity(Value(NullTag{}), ...)` passes `null` for `x` and
 *    `undefined` for `z`; the real `Entity.set_velocity` rejects both identically, so
 *    only the `y` argument is observable here.
 * 6. When `data.indexes` is absent the TS original hands `undefined` to
 *    `enter_frame_by_id` while the port hands `to_string(Value())`; the case always
 *    supplies the index, so this contract-external divergence is not covered.
 */
export default {
  subject: "buff_magic_flute",
  mutations: [
    {
      note: "MagicFlute injury is three",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFluteInjury = 2.0;",
      to: "constexpr double kFluteInjury = 3.0;",
    },
    {
      note: "MagicFlute injury_r is a quarter",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFluteInjuryR = 0.5;",
      to: "constexpr double kFluteInjuryR = 0.25;",
    },
    {
      note: "MagicFlute2 injury is two",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFlute2Injury = 1.0;",
      to: "constexpr double kFlute2Injury = 2.0;",
    },
    {
      note: "MagicFlute2 injury_r is a quarter",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFlute2InjuryR = 0.5;",
      to: "constexpr double kFlute2InjuryR = 0.25;",
    },
    {
      note: "fall injury is twenty one",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFallInjury = 20.0;",
      to: "constexpr double kFallInjury = 21.0;",
    },
    {
      note: "toughness is reset to one",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kToughness = 0.0;",
      to: "constexpr double kToughness = 1.0;",
    },
    {
      note: "MagicFlute acceleration is four",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFluteAcc = 3.0;",
      to: "constexpr double kFluteAcc = 4.0;",
    },
    {
      note: "MagicFlute2 acceleration is two and a half",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kFlute2Acc = 1.5;",
      to: "constexpr double kFlute2Acc = 2.5;",
    },
    {
      note: "velocity decay uses another factor",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "constexpr double kDecay = 0.25;",
      to: "constexpr double kDecay = 0.5;",
    },
    {
      note: "hp_r grows instead of dropping",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_hp_r(Value(to_number(victim->hp_r()) - injury_r));",
      to: "victim->set_hp_r(Value(to_number(victim->hp_r()) + injury_r));",
    },
    {
      note: "hp grows instead of dropping",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_hp(Value(to_number(victim->hp()) - injury));",
      to: "victim->set_hp(Value(to_number(victim->hp()) + injury));",
    },
    {
      note: "hp is not changed at all",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_hp(Value(to_number(victim->hp()) - injury));\n",
      to: "",
    },
    {
      note: "hp_r is not changed at all",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_hp_r(Value(to_number(victim->hp_r()) - injury_r));\n",
      to: "",
    },
    {
      note: "the previous hp is read from hp_r",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "const Value prev_hp = victim->hp();",
      to: "const Value prev_hp = victim->hp_r();",
    },
    {
      note: "fallinjury is written into toughness",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_fallinjury(Value(kFallInjury));",
      to: "victim->set_toughness(Value(kFallInjury));",
    },
    {
      note: "toughness is written into fallinjury",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_toughness(Value(kToughness));",
      to: "victim->set_fallinjury(Value(kToughness));",
    },
    {
      note: "fallinjury is not written",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_fallinjury(Value(kFallInjury));\n",
      to: "",
    },
    {
      note: "toughness is not reset",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "victim->set_toughness(Value(kToughness));\n",
      to: "",
    },
    {
      note: "the damage summary only runs without an attacker",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (attacker != nullptr) {\n    g_env.summary_apply_damage(attacker, Value(injury), victim, prev_hp);\n  }",
      to: "  if (attacker == nullptr) {\n    g_env.summary_apply_damage(attacker, Value(injury), victim, prev_hp);\n  }",
    },
    {
      note: "the damage summary swaps injury and previous hp",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    g_env.summary_apply_damage(attacker, Value(injury), victim, prev_hp);",
      to: "    g_env.summary_apply_damage(attacker, prev_hp, victim, Value(injury));",
    },
    {
      note: "the damage summary is never reported",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (attacker != nullptr) {\n    g_env.summary_apply_damage(attacker, Value(injury), victim, prev_hp);\n  }\n",
      to: "",
    },
    {
      note: "the falling frame is read from the down facing",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: 'to_string(index_0(index_by(victim->data_indexes_falling(), u"-1"))));',
      to: 'to_string(index_0(index_by(victim->data_indexes_falling(), u"1"))));',
    },
    {
      note: "the falling frame takes the second entry",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() == 0) return Value();\n    return arr->at(1);",
    },
    {
      note: "index_0 rejects every non empty list",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() > 0) return Value();\n    return arr->at(0);",
    },
    {
      note: "index_by ignores the found entry",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    const Value* p = o->get(k);\n    if (p != nullptr) return *p;",
      to: "    const Value* p = o->get(k);\n    if (p != nullptr) return Value();",
    },
    {
      note: "the falling list is not indexed",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: 'to_string(index_0(index_by(victim->data_indexes_falling(), u"-1"))));',
      to: 'to_string(index_by(victim->data_indexes_falling(), u"-1")));',
    },
    {
      note: "the velocity is no longer computed by calc_v",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  const double vy = entity::calc_v(victim->velocity_y(), acc,\n                                   Value(static_cast<double>(SpeedMode::AccTo)), Value(acc),\n                                   Value(1.0));",
      to: "  const double vy = entity::calc_v(victim->velocity_y(), acc,\n                                   Value(static_cast<double>(SpeedMode::AccTo)), Value(0.0),\n                                   Value(1.0));",
    },
    {
      note: "the velocity interpolation uses the default mode",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "Value(static_cast<double>(SpeedMode::AccTo)), Value(acc),",
      to: "Value(static_cast<double>(SpeedMode::Default)), Value(acc),",
    },
    {
      note: "the velocity direction is reversed",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "                                   Value(1.0));\n  victim->set_velocity(Value(NullTag{}), Value(vy), Value());",
      to: "                                   Value(-1.0));\n  victim->set_velocity(Value(NullTag{}), Value(vy), Value());",
    },
    {
      note: "current and target velocity are swapped",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  const double vy = entity::calc_v(victim->velocity_y(), acc,",
      to: "  const double vy = entity::calc_v(acc, victim->velocity_y(),",
    },
    {
      note: "the computed velocity is written into x",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  victim->set_velocity(Value(NullTag{}), Value(vy), Value());",
      to: "  victim->set_velocity(Value(vy), Value(NullTag{}), Value());",
    },
    {
      note: "the velocity decay is never applied",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  victim->handle_velocity_decay(kDecay);\n",
      to: "",
    },
    {
      note: "the fighter type test is loose",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
      to: "  if (equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
    },
    {
      note: "the weapon type test is loose",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {",
      to: "  if (equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {",
    },
    {
      note: "the fighter arm accepts weapons",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
      to: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {",
    },
    {
      note: "the weapon arm accepts fighters",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {",
      to: "  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
    },
    {
      note: "the world is not checked before the fall frame",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Falling)))) {",
      to: "    if (strict_equals(state, Value(static_cast<double>(StateEnum::Falling)))) {",
    },
    {
      note: "the falling state test is loose",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Falling)))) {",
      to: "    if (!equals(state, Value(static_cast<double>(StateEnum::Falling)))) {",
    },
    {
      note: "the sky states are combined with or",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Weapon_InTheSky))) &&\n        !strict_equals(state, Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)))) {",
      to: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Weapon_InTheSky))) ||\n        !strict_equals(state, Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)))) {",
    },
    {
      note: "the heavy sky state is not tested",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "        !strict_equals(state, Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)))) {",
      to: "        !strict_equals(state, Value(static_cast<double>(StateEnum::Weapon_InTheSky)))) {",
    },
    {
      note: "the light sky state is not tested",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Weapon_InTheSky))) &&",
      to: "    if (!strict_equals(state, Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky))) &&",
    },
    {
      note: "the team is copied from the victim",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "      if (attacker != nullptr) victim->set_team(attacker->team());",
      to: "      if (attacker != nullptr) victim->set_team(victim->team());",
    },
    {
      note: "the team is copied only without an attacker",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "      if (attacker != nullptr) victim->set_team(attacker->team());",
      to: "      if (attacker == nullptr) victim->set_team(attacker->team());",
    },
    {
      note: "the team is never copied",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "      if (attacker != nullptr) victim->set_team(attacker->team());\n",
      to: "",
    },
    {
      note: "the weapon enters the falling frame",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "      victim->enter_frame_by_id(to_string(index_0(victim->data_indexes_in_the_skys())));",
      to: "      victim->enter_frame_by_id(to_string(index_0(victim->data_indexes_falling())));",
    },
    {
      note: "the weapon does not enter a frame",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "      victim->enter_frame_by_id(to_string(index_0(victim->data_indexes_in_the_skys())));\n",
      to: "",
    },
    {
      note: "the fighter does not enter a frame",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: '      victim->enter_frame_by_id(\n          to_string(index_0(index_by(victim->data_indexes_falling(), u"-1"))));\n',
      to: "",
    },
    {
      note: "init keeps the default duration",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "void Buff_MagicFlute::init() {\n  set_ticks(3);\n  set_duration(3);\n}",
      to: "void Buff_MagicFlute::init() {\n  set_ticks(3);\n}",
    },
    {
      note: "init changes the tick interval",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "void Buff_MagicFlute::init() {\n  set_ticks(3);",
      to: "void Buff_MagicFlute::init() {\n  set_ticks(4);",
    },
    {
      note: "the second class inits with the first class ticks",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "void Buff_MagicFlute2::init() {\n  set_ticks(3);\n  set_duration(3);\n}",
      to: "void Buff_MagicFlute2::init() {\n  set_ticks(4);\n  set_duration(3);\n}",
    },
    {
      note: "the fighter on_tick swaps injury and injury_r",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  apply_flute_tick(attacker, victim, kFluteInjury, kFluteInjuryR);",
      to: "  apply_flute_tick(attacker, victim, kFluteInjuryR, kFluteInjury);",
    },
    {
      note: "the second class on_tick swaps injury and injury_r",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  apply_flute_tick(attacker, victim, kFlute2Injury, kFlute2InjuryR);",
      to: "  apply_flute_tick(attacker, victim, kFlute2InjuryR, kFlute2Injury);",
    },
    {
      note: "the fighter on_update uses the second class acceleration",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  apply_flute_update(attacker, victim, kFluteAcc);",
      to: "  apply_flute_update(attacker, victim, kFlute2Acc);",
    },
    {
      note: "the second class on_update uses the first class acceleration",
      file: "native/lfw/buff/buff_magic_flute.cpp",
      from: "  apply_flute_update(attacker, victim, kFlute2Acc);",
      to: "  apply_flute_update(attacker, victim, kFluteAcc);",
    },
    {
      note: "the fighter on_tick is never scheduled",
      file: "native/lfw/buff/buff_magic_flute.h",
      from: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return true; }",
      to: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return false; }",
    },
    {
      note: "the fighter on_update is never scheduled",
      file: "native/lfw/buff/buff_magic_flute.h",
      from: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return true; }",
      to: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return false; }\n  bool has_on_tick() const override { return true; }",
    },
    {
      note: "the second class on_tick is never scheduled",
      file: "native/lfw/buff/buff_magic_flute.h",
      from: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute2);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return true; }",
      to: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute2);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return false; }",
    },
    {
      note: "the second class on_update is never scheduled",
      file: "native/lfw/buff/buff_magic_flute.h",
      from: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute2);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return true; }\n  bool has_on_tick() const override { return true; }",
      to: "  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute2);\n  using Buff::Buff;\n  void init() override;\n\n protected:\n  bool has_on_update() const override { return false; }\n  bool has_on_tick() const override { return true; }",
    },
  ],
};
