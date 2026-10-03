// Mutation spec for the `weapon_state_base` differential slice.
//
// Subject: native/lfw/state/weapon_state_base.{h,cpp} + native/lfw/defines/weapon_bounce.h
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `e.hp = ...`, `e.hp_r = ...` and `e.drop_hurted = true` are plain property
//    WRITES in the original; the port routes them through `set_hp` / `set_hp_r` /
//    `set_drop_hurted` seams which stay SILENT on both harnesses, so those values
//    are observed through the state text instead.
//  * the `WT_*` tables of `Defines` are indexed by the weapon type; the entries
//    for `Heavy` (fast_y / fast_x / fast_z == 1) are provably unreachable in the
//    align branch, because `fast == 1` contradicts the bounce gate
//    (`min_y == min_x == 2`, `min_z == 99`). Mutating those three Heavy entries
//    is therefore unobservable and intentionally absent.
//  * `weapon_bounce_index` returning -1 must NOT be mutated into "keep going":
//    a negative index would read out of bounds of the fixed size table (UB), not
//    a visible divergence.
//  * `index_0`'s `if (arr->size() == 0) return Value();` guard must not be
//    dropped either: `arr->at(0)` on an empty array is UB.
//  * `Defines.WT_BOUNCE_Z` and `Defines.WT_BOUNCE_X` hold the SAME value for
//    every weapon type (0.5 / 0.5 / 0.75 / 0.5 / 0.75 / 0.75), so routing the
//    depth rebound through `wt_bounce_x` is unobservable and intentionally
//    absent from the list.

export default {
  subject: "weapon_state_base",
  mutations: [
    {
      note: "the ground bounce threshold is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{2, 2, 2, 2, 1, 1}},",
      to: "    {{2, 2, 0.2, 2, 1, 1}},",
    },
    {
      note: "the lateral bounce threshold never matches",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{99, 99, 2, 99, 2, 2}},",
      to: "    {{99, 99, 99, 99, 2, 2}},",
    },
    {
      note: "the z bounce threshold accepts a still weapon",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{99, 99, 99, 99, 99, 99}},",
      to: "    {{99, 99, -1, 99, 99, 99}},",
    },
    {
      note: "the ground restitution is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{0.5, 0.2, 0.3, 0.2, 0.45, 0.45}},",
      to: "    {{0.5, 0.2, 30, 0.2, 0.45, 0.45}},",
    },
    {
      note: "the lateral restitution is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{0.5, 0.2, 0.3, 0.2, 0.45, 0.45}},\n    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},",
      to: "    {{0.5, 0.2, 0.3, 0.2, 0.45, 0.45}},\n    {{0.5, 0.5, 2, 0.5, 0.75, 0.75}},",
    },
    {
      note: "the depth restitution is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},\n    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},",
      to: "    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},\n    {{0.5, 0.5, 0.75, 0.5, 2, 0.75}},",
    },
    {
      note: "the ground fast threshold is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{99, 99, 1, 99, 99, 99}},\n    {{99, 99, 1, 99, 4.5, 4.5}},",
      to: "    {{99, 99, 1, 99, 1, 99}},\n    {{99, 99, 1, 99, 4.5, 4.5}},",
    },
    {
      note: "the lateral fast threshold is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{99, 99, 1, 99, 99, 99}},\n    {{99, 99, 1, 99, 4.5, 4.5}},",
      to: "    {{99, 99, 1, 99, 99, 99}},\n    {{99, 99, 1, 99, -1, 4.5}},",
    },
    {
      note: "the depth fast threshold is wrong",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "    {{99, 99, 1, 99, 99, 99}},\n    {{99, 99, 1, 99, 4.5, 4.5}},\n    {{99, 99, 1, 99, 99, 99}},",
      to: "    {{99, 99, 1, 99, 99, 99}},\n    {{99, 99, 1, 99, 4.5, 4.5}},\n    {{99, 99, 1, 99, -1, 99}},",
    },
    {
      note: "the drink weapon type is not a table key",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "  if (!(d >= 0.0 && d <= 5.0)) return -1;",
      to: "  if (!(d >= 0.0 && d < 5.0)) return -1;",
    },
    {
      note: "a fractional weapon type is accepted as a table key",
      file: "native/lfw/defines/weapon_bounce.h",
      from: "  if (static_cast<double>(i) != d) return -1;",
      to: "  if (false) return -1;",
    },
    {
      note: "the indexes guard is dropped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (!e.has_data_indexes()) return Value();",
      to: "  if (!e.has_data_indexes() && false) return Value();",
    },
    {
      note: "the on ground test is inverted",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (e.is_on_ground()) return index_by(e.data_frames(), to_string(e.data_indexes_on_ground()));",
      to: "  if (!e.is_on_ground()) return index_by(e.data_frames(), to_string(e.data_indexes_on_ground()));",
    },
    {
      note: "the ground frame key is the sky one",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (e.is_on_ground()) return index_by(e.data_frames(), to_string(e.data_indexes_on_ground()));",
      to: "  if (e.is_on_ground()) return index_by(e.data_frames(), to_string(e.data_indexes_throwings()));",
    },
    {
      note: "the sky frame key is not taken from the list",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  return index_by(e.data_frames(), to_string(index_0(e.data_indexes_in_the_skys())));",
      to: "  return index_by(e.data_frames(), to_string(e.data_indexes_on_ground()));",
    },
    {
      note: "the frame table is not consulted for the sky frame",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  return index_by(e.data_frames(), to_string(index_0(e.data_indexes_in_the_skys())));",
      to: "  return Value(to_string(index_0(e.data_indexes_in_the_skys())));",
    },
    {
      note: "the landing frame is ignored",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  if (false) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "the landing frame is always used",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  if (true) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "a falsy landing frame is still entered",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    e.enter_frame(landing);\n    return;",
      to: "    e.enter_frame(Value(false));\n    return;",
    },
    {
      note: "the landing fallback uses the wrong index",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_on_ground()));",
      to: "  e.enter_frame_by_id(to_string(e.data_indexes_throwings()));",
    },
    {
      note: "leaving the ground enters the wrong frame",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));",
      to: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto) + u\"x\")));",
    },
    {
      note: "leaving the ground does not build a next frame",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));",
      to: "  e.enter_frame(Value(std::u16string(frame_id::kAuto)));",
    },
    {
      note: "the ground velocity decay is skipped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "void WeaponState_Base::update(IStateEntity& e) {\n  e.handle_ground_velocity_decay();\n}",
      to: "void WeaponState_Base::update(IStateEntity& e) {\n  (void)e;\n}",
    },
    {
      note: "the velocity y is read from x",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double vy = to_number(field_or(velocity, u\"y\"));",
      to: "  const double vy = to_number(field_or(velocity, u\"x\"));",
    },
    {
      note: "the velocity x is read from z",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double vx = to_number(field_or(velocity, u\"x\"));",
      to: "  const double vx = to_number(field_or(velocity, u\"z\"));",
    },
    {
      note: "the velocity z is read from y",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double vz = to_number(field_or(velocity, u\"z\"));",
      to: "  const double vz = to_number(field_or(velocity, u\"y\"));",
    },
    {
      note: "the data base overrides are ignored",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value base = e.data_base();",
      to: "  const Value base = Value();",
    },
    {
      note: "the weapon type is ignored",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value wt = e.base_type();",
      to: "  const Value wt = Value();",
    },
    {
      note: "the lateral restitution ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value bounce_x =\n      coalesce2(field_or(base, u\"bounce_x\"), wt_bounce_x(wt), Value(0.5));",
      to: "  const Value bounce_x = Value(0.5);",
    },
    {
      note: "the lateral restitution is taken from the table unconditionally",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value bounce_x =\n      coalesce2(field_or(base, u\"bounce_x\"), wt_bounce_x(wt), Value(0.5));",
      to: "  const Value bounce_x = coalesce2(wt_bounce_x(wt), Value(0.5), Value(0.5));",
    },
    {
      note: "the ground restitution is taken from the lateral table",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value bounce_y =\n      coalesce2(field_or(base, u\"bounce_y\"), wt_bounce_y(wt), Value(0.5));",
      to: "  const Value bounce_y =\n      coalesce2(field_or(base, u\"bounce_y\"), wt_bounce_x(wt), Value(0.5));",
    },
    {

      note: "the ground bounce threshold ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value bounce_min_y =\n      coalesce2(field_or(base, u\"bounce_min_y\"), wt_bounce_min_y(wt), Value(0.5));",
      to: "  const Value bounce_min_y =\n      coalesce2(Value(0.5), wt_bounce_min_y(wt), Value(0.5));",
    },
    {
      note: "the lateral bounce threshold ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value bounce_min_x =\n      coalesce2(field_or(base, u\"bounce_min_x\"), wt_bounce_min_x(wt), Value(99.0));",
      to: "  const Value bounce_min_x =\n      coalesce2(Value(99.0), wt_bounce_min_x(wt), Value(99.0));",
    },
    {
      note: "the depth bounce threshold readers are swapped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      coalesce2(field_or(base, u\"bounce_min_z\"), wt_bounce_min_z(wt), Value(99.0));",
      to: "      coalesce2(field_or(base, u\"bounce_min_x\"), wt_bounce_min_z(wt), Value(99.0));",
    },
    {
      note: "the ground fast threshold ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value fast_y = coalesce2(field_or(base, u\"fast_vy\"), wt_fast_y(wt), Value(99.0));",
      to: "  const Value fast_y = coalesce2(Value(99.0), wt_fast_y(wt), Value(99.0));",
    },
    {
      note: "the lateral fast threshold ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), wt_fast_x(wt), Value(99.0));",
      to: "  const Value fast_x = coalesce2(Value(99.0), wt_fast_x(wt), Value(99.0));",
    },
    {
      note: "the depth fast threshold ignores the base override",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const Value fast_z = coalesce2(field_or(base, u\"fast_vz\"), wt_fast_z(wt), Value(99.0));",
      to: "  const Value fast_z = coalesce2(Value(99.0), wt_fast_z(wt), Value(99.0));",
    },
    {
      note: "the ground rebound keeps the incoming sign",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double dvy = round_float(-vy * to_number(bounce_y));",
      to: "  const double dvy = round_float(vy * to_number(bounce_y));",
    },
    {
      note: "the lateral rebound uses the ground restitution",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double dvx = round_float(vx * to_number(bounce_x));",
      to: "  const double dvx = round_float(vx * to_number(bounce_y));",
    },
    {

      note: "the ground rebound is not rounded",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double dvy = round_float(-vy * to_number(bounce_y));",
      to: "  const double dvy = -vy * to_number(bounce_y);",
    },
    {
      note: "the lateral rebound is not rounded",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double dvx = round_float(vx * to_number(bounce_x));",
      to: "  const double dvx = vx * to_number(bounce_x);",
    },
    {
      note: "the depth rebound is not rounded",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  const double dvz = round_float(vz * to_number(bounce_z));",
      to: "  const double dvz = vz * to_number(bounce_z);",
    },
    {
      note: "the drop hurt block always runs",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (!truthy(e.drop_hurted())) {",
      to: "  if (true) {",
    },
    {
      note: "the drop hurt block never runs",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (!truthy(e.drop_hurted())) {",
      to: "  if (false) {",
    },
    {
      note: "the drop hurt flag is cleared instead of set",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    e.set_drop_hurted(Value(true));",
      to: "    e.set_drop_hurted(Value(false));",
    },
    {
      note: "the drop hurt damage is not gated",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    if (truthy(field_or(base, u\"drop_hurt\"))) {",
      to: "    if (false) {",
    },
    {
      note: "the drop hurt heals instead of hurting",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      e.set_hp(Value(to_number(e.hp()) - to_number(hurt)));",
      to: "      e.set_hp(Value(to_number(e.hp()) + to_number(hurt)));",
    },
    {
      note: "the recoverable hp is reduced by the wrong field",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      e.set_hp_r(Value(to_number(e.hp_r()) - to_number(hurt)));",
      to: "      e.set_hp_r(Value(to_number(e.hp()) - to_number(hurt)));",
    },
    {
      note: "the recoverable hp is not reduced",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      e.set_hp_r(Value(to_number(e.hp_r()) - to_number(hurt)));",
      to: "      e.set_hp_r(Value(to_number(e.hp_r())));",
    },
    {
      note: "the ground bounce gate is strictly greater",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvy >= to_number(bounce_min_y) ||\n      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
      to: "      dvy > to_number(bounce_min_y) ||\n      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
    },
    {
      note: "the lateral bounce gate is strictly greater",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
      to: "      dvx > to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
    },
    {
      note: "the negative lateral bounce gate is inclusive",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
      to: "      dvx >= to_number(bounce_min_x) || dvx <= -to_number(bounce_min_x) ||",
    },
    {
      note: "the depth bounce gate reads the lateral speed",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx >= to_number(bounce_min_z) || dvx < -to_number(bounce_min_z);",
      to: "      dvz >= to_number(bounce_min_z) || dvz < -to_number(bounce_min_z);",
    },
    {
      note: "the depth bounce gate is dropped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx >= to_number(bounce_min_z) || dvx < -to_number(bounce_min_z);",
      to: "      false;",
    },
    {
      note: "the first two bounce gates are anded",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvy >= to_number(bounce_min_y) ||\n      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
      to: "      dvy >= to_number(bounce_min_y) &&\n      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||",
    },
    {
      note: "the bounce test is inverted",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (!is_bounce) {",
      to: "  if (is_bounce) {",
    },
    {
      note: "the non bounce frame is dropped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    e.enter_frame_by_id(to_string(nf));",
      to: "    e.enter_frame_by_id(to_string(Value()));",
    },
    {
      note: "the rebound velocity components are scrambled",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  e.set_velocity(Value(dvx), Value(dvy), Value(dvz));",
      to: "  e.set_velocity(Value(dvy), Value(dvx), Value(dvz));",
    },
    {
      note: "the rebound does not leave the ground",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  e.set_velocity(Value(dvx), Value(dvy), Value(dvz));\n  e.leave_ground();",
      to: "  e.set_velocity(Value(dvx), Value(dvy), Value(dvz));",
    },
    {
      note: "the throwing state test is strict",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (equals(e.state(), Value(static_cast<double>(StateEnum::Weapon_Throwing))) &&",
      to: "  if (strict_equals(e.state(), Value(static_cast<double>(StateEnum::Weapon_Throwing))) &&",
    },
    {
      note: "the align branch does not require the throwing state",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "  if (equals(e.state(), Value(static_cast<double>(StateEnum::Weapon_Throwing))) &&",
      to: "  if (equals(e.state(), e.state()) &&",
    },
    {
      note: "the ground fast gate is inclusive",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvy > -to_number(fast_y) && dvy < to_number(fast_y) &&",
      to: "      dvy >= -to_number(fast_y) && dvy < to_number(fast_y) &&",
    },
    {
      note: "the ground fast gate upper bound is inclusive",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvy > -to_number(fast_y) && dvy < to_number(fast_y) &&",
      to: "      dvy > -to_number(fast_y) && dvy <= to_number(fast_y) &&",
    },
    {
      note: "the lateral fast gate is inclusive",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx > -to_number(fast_x) && dvx < to_number(fast_x) &&",
      to: "      dvx >= -to_number(fast_x) && dvx < to_number(fast_x) &&",
    },
    {
      note: "the depth fast gate is inclusive",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvz > -to_number(fast_z) && dvz < to_number(fast_z)) {",
      to: "      dvz >= -to_number(fast_z) && dvz < to_number(fast_z)) {",
    },
    {
      note: "the ground fast gate reads the lateral threshold",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvy > -to_number(fast_y) && dvy < to_number(fast_y) &&",
      to: "      dvy > -to_number(fast_x) && dvy < to_number(fast_y) &&",
    },
    {
      note: "the lateral fast gate reads the ground threshold",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvx > -to_number(fast_x) && dvx < to_number(fast_x) &&",
      to: "      dvx > -to_number(fast_y) && dvx < to_number(fast_x) &&",
    },
    {
      note: "the depth fast gate reads the lateral threshold",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "      dvz > -to_number(fast_z) && dvz < to_number(fast_z)) {",
      to: "      dvz > -to_number(fast_x) && dvz < to_number(fast_z)) {",
    },
    {
      note: "the align source frame is not the current one",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_throwings(),",
      to: "    const Value align = e.find_align_frame(Value(), e.data_indexes_throwings(),",
    },
    {
      note: "the throwing and sky index arguments are swapped",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_throwings(),\n                                          e.data_indexes_in_the_skys());",
      to: "    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_in_the_skys(),\n                                          e.data_indexes_throwings());",
    },
    {
      note: "the align search reads the ground index",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_throwings(),",
      to: "    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_on_ground(),",
    },
    {
      note: "the align frame is entered unconditionally",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    if (truthy(align)) e.enter_frame(align);",
      to: "    if (true) e.enter_frame(align);",
    },
    {
      note: "the align frame is never entered",
      file: "native/lfw/state/weapon_state_base.cpp",
      from: "    if (truthy(align)) e.enter_frame(align);",
      to: "    if (truthy(align)) e.enter_frame(Value());",
    },
  ],
};
