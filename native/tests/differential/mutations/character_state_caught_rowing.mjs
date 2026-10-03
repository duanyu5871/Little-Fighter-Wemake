/**
 * Mutation spec for `native/lfw/state/character_state_caught_rowing.{h,cpp}`
 * (`CharacterState_Caught` / `CharacterState_Rowing`).
 *
 * Disproven / unobservable mutations (proven by reading the TS originals, the port
 * and the case file, not by a surviving run):
 *
 * 1. The two constructors' default arguments (`StateEnum.Caught` / `StateEnum.Rowing`)
 *    are unobservable: nothing in this unit reads the state object's own `_state`.
 * 2. `CharacterState_Caught.update` does **not** call `super.update(e)`, so the base
 *    class' `handle_ground_velocity_decay` is never driven for this state; the port
 *    deliberately omits the call and the case asserts that `run update` logs nothing.
 * 3. `calc_v`'s `acc` argument is unused by `SpeedMode.Default` (it only matters for
 *    the `Acc` / `AccTo` / `FixedAcc*` branches), so mutating `Value(0.0)` there is
 *    unobservable.
 * 4. Multiplication is commutative, so swapping the two dataset operands of `vx` /
 *    `vy` is unobservable.
 * 5. A **missing** `velocity.y` cannot be represented: the port's `velocity_y()` seam
 *    (and `calc_v` itself) is `double`-typed, so a missing y is `NaN` where the TS
 *    original keeps `undefined` and returns it from `calc_v`. The case therefore only
 *    feeds numeric `vely` values. (`velocity.x` is `Value`-typed and *is* covered with
 *    `u`.)
 * 6. `if (holding) e.drop_holding();` and the team-adoption block are two separate
 *    statements in the TS original; only re-ordering *inside* the second one would be
 *    a single-text mutation, and that block has just one statement.
 */
export default {
  subject: "character_state_caught_rowing",
  mutations: [
    {
      note: "caught does not reset the fall value",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_fall_value(e.fall_value_max());",
      to: "  e.set_fall_value(Value());",
    },
    {
      note: "caught never writes the fall value",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_fall_value(e.fall_value_max());\n",
      to: "",
    },
    {
      note: "caught reads the current fall value as the cap",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_fall_value(e.fall_value_max());",
      to: "  e.set_fall_value(e.fall_value());",
    },
    {
      note: "caught enters with a non zero velocity",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));\n  const bool holding = e.has_holding();",
      to: "  e.set_velocity(Value(1.0), Value(0.0), Value(0.0));\n  const bool holding = e.has_holding();",
    },
    {
      note: "caught enter zeroes nothing",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));\n  const bool holding = e.has_holding();",
      to: "  const bool holding = e.has_holding();",
    },
    {
      note: "caught always believes there is a holding",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const bool holding = e.has_holding();",
      to: "  const bool holding = true;",
    },
    {
      note: "caught never drops the holding",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (holding) e.drop_holding();\n",
      to: "",
    },
    {
      note: "caught adopts the team without a holding",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (holding &&\n      strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "the heavy holding test is loose",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "      strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "      equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "another weapon type is treated as heavy",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "      strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "      strict_equals(e.holding_base_type(), Value(1.0))) {",
    },
    {
      note: "the holding adopts no team",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "    e.holding_set_team(e.team());",
      to: "    e.holding_set_team(Value());",
    },
    {
      note: "the holding keeps its old team",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "    e.holding_set_team(e.team());\n",
      to: "",
    },
    {
      note: "caught update enters with a non zero velocity",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "void CharacterState_Caught::update(IStateEntity& e) {\n  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));\n}",
      to: "void CharacterState_Caught::update(IStateEntity& e) {\n  e.set_velocity(Value(0.0), Value(1.0), Value(0.0));\n}",
    },
    {
      note: "caught update does nothing",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "void CharacterState_Caught::update(IStateEntity& e) {\n  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));\n}",
      to: "void CharacterState_Caught::update(IStateEntity& e) {\n  (void)e;\n}",
    },
    {
      note: "rowing landing ignores the frame on_landing",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }\n",
      to: "",
    },
    {
      note: "rowing landing prefers the index over the frame on_landing",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  if (!truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "rowing landing falls back to the default frame",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_1()));",
      to: "  e.enter_frame_by_id(to_string(e.data_indexes_default()));",
    },
    {
      note: "rowing landing never enters a frame",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_1()));\n",
      to: "",
    },
    {
      note: "rowing accepts any previous state",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (!strict_equals(lfw::field_or(prev_frame, u\"state\"),\n                     Value(static_cast<double>(StateEnum::Falling)))) {\n    return;\n  }\n",
      to: "",
    },
    {
      note: "rowing rejects a falling previous state",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (!strict_equals(lfw::field_or(prev_frame, u\"state\"),",
      to: "  if (strict_equals(lfw::field_or(prev_frame, u\"state\"),",
    },
    {
      note: "the previous state test is loose",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (!strict_equals(lfw::field_or(prev_frame, u\"state\"),",
      to: "  if (!equals(lfw::field_or(prev_frame, u\"state\"),",
    },
    {
      note: "rowing requires another previous state",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "                     Value(static_cast<double>(StateEnum::Falling)))) {",
      to: "                     Value(static_cast<double>(StateEnum::Caught)))) {",
    },
    {
      note: "the previous frame field is misspelled",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  if (!strict_equals(lfw::field_or(prev_frame, u\"state\"),",
      to: "  if (!strict_equals(lfw::field_or(prev_frame, u\"statee\"),",
    },
    {
      note: "the x rowing distance is subtracted",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const double vx = to_number(rowing_distance) * to_number(bfall_x_f);",
      to: "  const double vx = to_number(rowing_distance) + to_number(bfall_x_f);",
    },
    {
      note: "the y rowing height is subtracted",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const double vy = to_number(rowing_height) * to_number(bfall_h_f);",
      to: "  const double vy = to_number(rowing_height) + to_number(bfall_h_f);",
    },
    {
      note: "the x distance reads the height dataset",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: '  const Value rowing_distance = e.dataset(u"rowing_distance");',
      to: '  const Value rowing_distance = e.dataset(u"rowing_height");',
    },
    {
      note: "the x factor reads the y factor dataset",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: '  const Value bfall_x_f = e.dataset(u"bfall_x_f");',
      to: '  const Value bfall_x_f = e.dataset(u"bfall_h_f");',
    },
    {
      note: "the y height reads the distance dataset",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: '  const Value rowing_height = e.dataset(u"rowing_height");',
      to: '  const Value rowing_height = e.dataset(u"rowing_distance");',
    },
    {
      note: "the y factor reads the x factor dataset",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: '  const Value bfall_h_f = e.dataset(u"bfall_h_f");',
      to: '  const Value bfall_h_f = e.dataset(u"bfall_x_f");',
    },
    {
      note: "a zero x velocity counts as positive",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const Value next_vx = to_number(prev_vx) >= 0 ? Value(vx) : Value(-vx);",
      to: "  const Value next_vx = to_number(prev_vx) > 0 ? Value(vx) : Value(-vx);",
    },
    {
      note: "the rowing direction is not flipped",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const Value next_vx = to_number(prev_vx) >= 0 ? Value(vx) : Value(-vx);",
      to: "  const Value next_vx = to_number(prev_vx) >= 0 ? Value(-vx) : Value(vx);",
    },
    {
      note: "the previous y velocity is not read",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  const double prev_vy = e.velocity_y();",
      to: "  const double prev_vy = 0;",
    },
    {
      note: "the y interpolation uses another mode",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "      entity::calc_v(prev_vy, vy, Value(static_cast<double>(SpeedMode::Default)), Value(0.0),\n                     Value(1.0));",
      to: "      entity::calc_v(prev_vy, vy, Value(static_cast<double>(SpeedMode::Fixed)), Value(0.0),\n                     Value(1.0));",
    },
    {
      note: "the velocity axes are swapped",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_velocity(next_vx, Value(next_vy), Value());",
      to: "  e.set_velocity(Value(next_vy), next_vx, Value());",
    },
    {
      note: "the y velocity is dropped",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_velocity(next_vx, Value(next_vy), Value());",
      to: "  e.set_velocity(next_vx, Value(), Value());",
    },
    {
      note: "rowing writes no velocity",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  e.set_velocity(next_vx, Value(next_vy), Value());\n",
      to: "",
    },
    {
      note: "the caught enter hook is not installed",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  enter = &csc_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the rowing landing hook is not installed",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  on_landing = &csr_on_landing;",
      to: "  on_landing = nullptr;",
    },
    {
      note: "the rowing enter hook is not installed",
      file: "native/lfw/state/character_state_caught_rowing.cpp",
      from: "  enter = &csr_enter;",
      to: "  enter = nullptr;",
    },
  ],
};
