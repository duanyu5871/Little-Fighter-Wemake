/**
 * Mutation spec for `native/lfw/state/character_state_basic.{h,cpp}`
 * (`CharacterState_Standing` / `CharacterState_Running` / `CharacterState_Injured`).
 *
 * Disproven / unobservable mutations (proven by reading the TS originals, the port
 * and the case file, not by a surviving run):
 *
 * 1. The three constructors' **default arguments** (`StateEnum.Standing` /
 *    `StateEnum.Running` / `StateEnum.Injured`) are unobservable here: none of the
 *    three classes ever reads the state object's own `_state` (`leave` /
 *    `on_restrict` are not called by this unit's case). The `usedefault 1` scenarios
 *    only prove that the constructor can be called without an argument.
 * 2. `CharacterState_Injured`'s `super.enter?.(e, prev_frame)` is always a no-op:
 *    neither `State_Base.ts` nor `CharacterState_Base.ts` **implements** `enter`
 *    (both only declare it with `?`), so the optional call can never resolve to a
 *    function.
 * 3. `index_0`'s object branch is dead here (`in_the_skys` is always an array).
 * 4. `if (!truthy(fid)) return Value();`-style guards have no counterpart in these
 *    three classes; the analogous `enter_frame_by_id(to_string(undefined))` path
 *    (missing `in_the_skys`) hands `""` where the TS original hands `undefined`, so
 *    the case always supplies the index.
 * 5. `e.set_velocity(vx)` passes only `x`; `y`/`z` are omitted and the harness
 *    reproduces the real `Entity.set_velocity` rule (skip `null` / `undefined`), so
 *    only `x` is observable through the state text.
 */
export default {
  subject: "character_state_basic",
  mutations: [
    {
      note: "the sky frame takes the second entry",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() == 0) return Value();\n    return arr->at(1);",
    },
    {
      note: "index_0 rejects every non empty list",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() > 0) return Value();\n    return arr->at(0);",
    },
    {
      note: "the heavy holding test is inverted",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;",
      to: "  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;",
    },
    {
      note: "the heavy holding test is loose",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;",
      to: "  if (!equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;",
    },
    {
      note: "another weapon type is treated as heavy",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;",
      to: "  if (!strict_equals(e.holding_base_type(), Value(1.0))) return;",
    },
    {
      note: "the heavy holding guard is skipped",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;\n",
      to: "",
    },
    {
      note: "the holding is not dropped",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  e.drop_holding();\n",
      to: "",
    },
    {
      note: "the holding adopts no team",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  e.holding_set_team(e.team());",
      to: "  e.holding_set_team(Value());",
    },
    {
      note: "the holding adopts the facing as team",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  e.holding_set_team(e.team());",
      to: "  e.holding_set_team(e.facing());",
    },
    {
      note: "the holding keeps its old team",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  e.holding_set_team(e.team());\n",
      to: "",
    },
    {
      note: "the team is adopted before the drop",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  e.drop_holding();\n  e.holding_set_team(e.team());",
      to: "  e.holding_set_team(e.team());\n  e.drop_holding();",
    },
    {
      note: "standing does not decay the ground velocity",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "void CharacterState_Standing::update(IStateEntity& e) {\n  CharacterState_Base::update(e);",
      to: "void CharacterState_Standing::update(IStateEntity& e) {",
    },
    {
      note: "standing ignores a zero hp",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (to_number(e.hp()) <= 0) {\n    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
      to: "  if (to_number(e.hp()) < 0) {\n    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
    },
    {
      note: "standing treats a positive hp as dead",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (to_number(e.hp()) <= 0) {\n    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
      to: "  if (to_number(e.hp()) > 0) {\n    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
    },
    {
      note: "standing falls through the sudden death frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
      to: "    e.enter_frame(e.get_sudden_death_frame());\n  }",
    },
    {
      note: "standing enters an empty sudden death frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.enter_frame(e.get_sudden_death_frame());",
      to: "    e.enter_frame(Value());",
    },
    {
      note: "standing never enters the sudden death frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }\n",
      to: "    return;\n  }\n",
    },
    {
      note: "standing compares the wrong position axis",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (px > to_number(e.ground_y())) {",
    },
    {
      note: "standing accepts being exactly on the ground",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (py >= to_number(e.ground_y())) {",
    },
    {
      note: "standing compares the hp as the ground",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (py > to_number(e.hp())) {",
    },
    {
      note: "standing uses the default frame instead of the sky frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(e.data_indexes_in_the_skys())));",
      to: "    e.enter_frame_by_id(to_string(e.data_indexes_default()));",
    },
    {
      note: "standing never enters the sky frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(e.data_indexes_in_the_skys())));\n",
      to: "",
    },
    {
      note: "running does not decay the ground velocity",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "void CharacterState_Running::update(IStateEntity& e) {\n  CharacterState_Base::update(e);",
      to: "void CharacterState_Running::update(IStateEntity& e) {",
    },
    {
      note: "running drags with another divisor",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    const double dz = abs(to_number(vz) / 4);",
      to: "    const double dz = abs(to_number(vz) / 2);",
    },
    {
      note: "running drags with the raw z velocity",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    const double dz = abs(to_number(vz) / 4);",
      to: "    const double dz = to_number(vz) / 4;",
    },
    {
      note: "running ignores the drag on a still entity",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (truthy(vz)) {",
      to: "  if (!truthy(vz)) {",
    },
    {
      note: "running always drags",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (truthy(vz)) {\n    const double dz",
      to: "  if (true) {\n    const double dz",
    },
    {
      note: "running adds the drag to a positive x",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (to_number(vx) > dz) vx = Value(to_number(vx) - dz);",
      to: "    if (to_number(vx) > dz) vx = Value(to_number(vx) + dz);",
    },
    {
      note: "running subtracts the drag from a negative x",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (to_number(vx) < -dz) vx = Value(to_number(vx) + dz);",
      to: "    if (to_number(vx) < -dz) vx = Value(to_number(vx) - dz);",
    },
    {
      note: "running never lowers a positive x",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (to_number(vx) > dz) vx = Value(to_number(vx) - dz);\n",
      to: "",
    },
    {
      note: "running never raises a negative x",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    if (to_number(vx) < -dz) vx = Value(to_number(vx) + dz);\n",
      to: "",
    },
    {
      note: "running writes the drag into z",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.set_velocity(vx, Value(), Value());",
      to: "    e.set_velocity(Value(), Value(), vx);",
    },
    {
      note: "running never writes the dragged velocity",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "    e.set_velocity(vx, Value(), Value());\n",
      to: "",
    },
    {
      note: "running ignores a zero hp",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (to_number(e.hp()) <= 0) e.enter_frame(e.get_sudden_death_frame());",
      to: "  if (to_number(e.hp()) < 0) e.enter_frame(e.get_sudden_death_frame());",
    },
    {
      note: "running never enters the sudden death frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (to_number(e.hp()) <= 0) e.enter_frame(e.get_sudden_death_frame());\n",
      to: "",
    },
    {
      note: "running enters an empty sudden death frame",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  if (to_number(e.hp()) <= 0) e.enter_frame(e.get_sudden_death_frame());",
      to: "  if (to_number(e.hp()) <= 0) e.enter_frame(Value());",
    },
    {
      note: "the injured enter hook is not installed",
      file: "native/lfw/state/character_state_basic.cpp",
      from: "  enter = &csi_enter;",
      to: "  enter = nullptr;",
    },
  ],
};
