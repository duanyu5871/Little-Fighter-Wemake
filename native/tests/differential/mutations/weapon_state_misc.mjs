// Mutation spec for the `weapon_state_misc` differential slice.
//
// Subject: native/lfw/state/weapon_state_misc.{h,cpp}
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `e.motionless` / `e.bearer` / `e.team` / `e.dropping` / `e.drop_hurted`
//    are plain property READS/WRITES in the original; the port routes them
//    through seams that stay SILENT on both harnesses (or are observed through
//    the state text), so only the *selection* between them is mutable.
//  * `WeaponState_OnHand`: `max(e.motionless, e.bearer.motionless)` is
//    symmetric (Math.max), so swapping the two arguments is unobservable and
//    intentionally absent; the read order of both operands is silent as well.
//  * `WeaponState_Throwing.enter`: the `leave_ground()` / `drop_hurted = false`
//    order is unobservable (the flag has no log), so only removing one of the
//    two calls is in the list.
//  * `Defines.WT_FAST_Y` / `WT_FAST_Z` are 99 for every non-Heavy weapon, and
//    the align gate excludes Heavy, so replacing `wt_fast_y(wt)` /
//    `wt_fast_z(wt)` with the final `99` fallback is unobservable and
//    intentionally absent. The `99` defaults themselves ARE mutable through
//    the `wt = 9` probes (y/z beyond 5, within 99).
//  * `hit_ground_rebouncing` is covered by the `weapon_state_base` slice; the
//    cases only exercise the delegation (`nf` argument, the inherited bounce
//    branch) without re-mutating the base method.
//  * Landing with an undefined `nf` is not covered by the cases (the C++
//    `enter_frame_by_id(std::u16string)` renders `undefined` as the string
//    "undefined" while TS renders the raw value), so the `||` chain is probed
//    with at least one non-undefined operand.

export default {
  subject: "weapon_state_misc",
  mutations: [
    // ---- WeaponState_OnGround ----
    {
      note: "the ground state constructor never installs enter",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "WeaponState_OnGround::WeaponState_OnGround(Value state) : WeaponState_Base(std::move(state)) {\n  enter = &wsog_enter;\n}",
      to: "WeaponState_OnGround::WeaponState_OnGround(Value state) : WeaponState_Base(std::move(state)) {\n  enter = nullptr;\n}",
    },
    {
      note: "the ground state constructor drops the state value",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "WeaponState_OnGround::WeaponState_OnGround(Value state) : WeaponState_Base(std::move(state)) {",
      to: "WeaponState_OnGround::WeaponState_OnGround(Value state) : WeaponState_Base(Value()) {",
    },
    {
      note: "the ground state enter keeps the old team",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.set_team(e.lfw_new_team());",
      to: "  e.set_team(e.team());",
    },
    {
      note: "the new team is not read",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.set_team(e.lfw_new_team());",
      to: "  e.set_team(Value());",
    },
    {
      note: "the ground state update skips the decay",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "void WeaponState_OnGround::update(IStateEntity& e) { e.handle_ground_velocity_decay(); }",
      to: "void WeaponState_OnGround::update(IStateEntity& e) { (void)e; }",
    },
    // ---- WeaponState_OnHand ----
    {
      note: "the hand state constructor never installs pre_update",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  pre_update = &wsoh_pre_update;",
      to: "  pre_update = nullptr;",
    },
    {
      note: "the falsy motionless guard is dropped",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  if (!truthy(motionless) || !e.has_bearer()) return;",
      to: "  if (!e.has_bearer()) return;",
    },
    {
      note: "the bearer guard is dropped",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  if (!truthy(motionless) || !e.has_bearer()) return;",
      to: "  if (!truthy(motionless)) return;",
    },
    {
      note: "the bearer takes the smaller motionless",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.set_bearer_motionless(Value(max(to_number(motionless), to_number(e.bearer_motionless()))));",
      to: "  e.set_bearer_motionless(Value(min(to_number(motionless), to_number(e.bearer_motionless()))));",
    },
    {
      note: "the motionless is written back to the weapon itself",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.set_bearer_motionless(Value(max(to_number(motionless), to_number(e.bearer_motionless()))));",
      to: "  e.set_motionless(Value(max(to_number(motionless), to_number(e.bearer_motionless()))));",
    },
    {
      note: "the bearer motionless is not read",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "to_number(e.bearer_motionless())",
      to: "to_number(motionless)",
    },
    // ---- WeaponState_Throwing ----
    {
      note: "the throwing constructor never installs get_gravity",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  get_gravity = &wst_get_gravity;",
      to: "  get_gravity = nullptr;",
    },
    {
      note: "the throwing constructor never installs enter",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  enter = &wst_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the throwing constructor drops the state value",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "WeaponState_Throwing::WeaponState_Throwing(Value state) : WeaponState_Base(std::move(state)) {",
      to: "WeaponState_Throwing::WeaponState_Throwing(Value state) : WeaponState_Base(Value()) {",
    },
    {
      note: "the boomerang test is inverted in get_gravity",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "Value wst_get_gravity(IStateEntity& e) {\n  if (is_boomerang(e)) {",
      to: "Value wst_get_gravity(IStateEntity& e) {\n  if (!is_boomerang(e)) {",
    },
    {
      note: "the boomerang test is strict",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  return equals(e.frame_behavior(), Value(static_cast<double>(FrameBehavior::Boomerang)));",
      to: "  return strict_equals(e.frame_behavior(), Value(static_cast<double>(FrameBehavior::Boomerang)));",
    },
    {
      note: "the gravity is not rounded",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    return Value(round_float(to_number(e.dataset(u\"weapon_throwing_gravity\")) / 4.0));",
      to: "    return Value(to_number(e.dataset(u\"weapon_throwing_gravity\")) / 4.0);",
    },
    {
      note: "the boomerang gravity multiplies instead of dividing",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    return Value(round_float(to_number(e.dataset(u\"weapon_throwing_gravity\")) / 4.0));",
      to: "    return Value(round_float(to_number(e.dataset(u\"weapon_throwing_gravity\")) * 4.0));",
    },
    {
      note: "the plain gravity is quartered too",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  return e.dataset(u\"weapon_throwing_gravity\");",
      to: "  return Value(round_float(to_number(e.dataset(u\"weapon_throwing_gravity\")) / 4.0));",
    },
    {
      note: "enter does not leave the ground",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.leave_ground();\n  e.set_drop_hurted(Value(false));",
      to: "  e.set_drop_hurted(Value(false));",
    },
    {
      note: "enter does not reset drop_hurted",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.leave_ground();\n  e.set_drop_hurted(Value(false));",
      to: "  e.leave_ground();",
    },
    {
      note: "enter sets drop_hurted instead of clearing it",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.leave_ground();\n  e.set_drop_hurted(Value(false));",
      to: "  e.leave_ground();\n  e.set_drop_hurted(Value(true));",
    },
    {
      note: "enter scales the velocity on the wrong behavior",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  e.set_drop_hurted(Value(false));\n  if (is_boomerang(e)) {",
      to: "  e.set_drop_hurted(Value(false));\n  if (!is_boomerang(e)) {",
    },
    {
      note: "enter reads the z velocity",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    e.set_velocity(Value(to_number(e.velocity_x()) * 0.6), Value(), Value());",
      to: "    e.set_velocity(Value(to_number(e.velocity_z()) * 0.6), Value(), Value());",
    },
    {
      note: "enter uses the wrong scale",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    e.set_velocity(Value(to_number(e.velocity_x()) * 0.6), Value(), Value());",
      to: "    e.set_velocity(Value(to_number(e.velocity_x()) * 0.4), Value(), Value());",
    },
    {
      note: "the throwing update skips the decay",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "void WeaponState_Throwing::update(IStateEntity& e) { e.handle_ground_velocity_decay(); }",
      to: "void WeaponState_Throwing::update(IStateEntity& e) { (void)e; }",
    },
    {
      note: "the throwing landing drops the velocity argument",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    wst_on_landing(*this, e, velocity);",
      to: "    wst_on_landing(*this, e, Value());",
    },
    {
      note: "the landing frame test is inverted",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value landing = e.frame_on_landing();\n  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }\n  // TS: indexes?.throw_on_ground || indexes?.just_on_ground",
      to: "  const Value landing = e.frame_on_landing();\n  if (!truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }\n  // TS: indexes?.throw_on_ground || indexes?.just_on_ground",
    },
    {
      note: "just_on_ground wins over throw_on_ground",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  self.hit_ground_rebouncing(e, truthy(throw_on_ground) ? throw_on_ground : just_on_ground,",
      to: "  self.hit_ground_rebouncing(e, truthy(just_on_ground) ? just_on_ground : throw_on_ground,",
    },
    {
      note: "throw_on_ground is ignored",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  self.hit_ground_rebouncing(e, truthy(throw_on_ground) ? throw_on_ground : just_on_ground,\n                             velocity);",
      to: "  self.hit_ground_rebouncing(e, just_on_ground,\n                             velocity);",
    },
    {
      note: "just_on_ground is ignored",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  self.hit_ground_rebouncing(e, truthy(throw_on_ground) ? throw_on_ground : just_on_ground,\n                             velocity);",
      to: "  self.hit_ground_rebouncing(e, throw_on_ground,\n                             velocity);",
    },
    // ---- WeaponState_InTheSky ----
    {
      note: "the sky state constructor never installs enter",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  enter = &wsis_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the sky state constructor drops the state value",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "WeaponState_InTheSky::WeaponState_InTheSky(Value state) : WeaponState_Base(std::move(state)) {",
      to: "WeaponState_InTheSky::WeaponState_InTheSky(Value state) : WeaponState_Base(Value()) {",
    },
    {
      note: "enter sets drop_hurted instead of clearing it",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "void wsis_enter(IStateEntity& e, const Value& prev_frame) {\n  (void)prev_frame;\n  e.set_drop_hurted(Value(false));\n}",
      to: "void wsis_enter(IStateEntity& e, const Value& prev_frame) {\n  (void)prev_frame;\n  e.set_drop_hurted(Value(true));\n}",
    },
    {
      note: "enter does not reset drop_hurted",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "void wsis_enter(IStateEntity& e, const Value& prev_frame) {\n  (void)prev_frame;\n  e.set_drop_hurted(Value(false));\n}",
      to: "void wsis_enter(IStateEntity& e, const Value& prev_frame) {\n  (void)prev_frame;\n}",
    },
    {
      note: "the sky landing uses throw_on_ground",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  self.hit_ground_rebouncing(e, e.data_indexes_just_on_ground(), velocity);",
      to: "  self.hit_ground_rebouncing(e, e.data_indexes_throw_on_ground(), velocity);",
    },
    {
      note: "the sky landing drops the velocity argument",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    wsis_on_landing(*this, e, velocity);",
      to: "    wsis_on_landing(*this, e, Value());",
    },
    {
      note: "the sky update skips the decay",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "void WeaponState_InTheSky::update(IStateEntity& e) {\n  e.handle_ground_velocity_decay();\n\n  const double vy = e.velocity_y();",
      to: "void WeaponState_InTheSky::update(IStateEntity& e) {\n\n  const double vy = e.velocity_y();",
    },
    {
      note: "the heavy guard is dropped",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  if (!equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&",
      to: "  if (true &&",
    },
    {
      note: "the heavy guard is inverted",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  if (!equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&",
      to: "  if (equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&",
    },
    {
      note: "the heavy guard is strict",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  if (!equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&",
      to: "  if (!strict_equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&",
    },
    {
      note: "the y lower bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "      (vy < -to_number(fast_y) || vy > to_number(fast_y) ||",
      to: "      (vy <= -to_number(fast_y) || vy > to_number(fast_y) ||",
    },
    {
      note: "the y upper bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "      (vy < -to_number(fast_y) || vy > to_number(fast_y) ||",
      to: "      (vy < -to_number(fast_y) || vy >= to_number(fast_y) ||",
    },
    {
      note: "the x lower bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vx < -to_number(fast_x) || vx > to_number(fast_x) ||",
      to: "       vx <= -to_number(fast_x) || vx > to_number(fast_x) ||",
    },
    {
      note: "the x upper bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vx < -to_number(fast_x) || vx > to_number(fast_x) ||",
      to: "       vx < -to_number(fast_x) || vx >= to_number(fast_x) ||",
    },
    {
      note: "the z lower bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vz < -to_number(fast_z) || vz > to_number(fast_z))) {",
      to: "       vz <= -to_number(fast_z) || vz > to_number(fast_z))) {",
    },
    {
      note: "the z upper bound is inclusive",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vz < -to_number(fast_z) || vz > to_number(fast_z))) {",
      to: "       vz < -to_number(fast_z) || vz >= to_number(fast_z))) {",
    },
    {
      note: "the y lower bound reads the x threshold",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "      (vy < -to_number(fast_y) || vy > to_number(fast_y) ||",
      to: "      (vy < -to_number(fast_x) || vy > to_number(fast_y) ||",
    },
    {
      note: "the x lower bound reads the y threshold",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vx < -to_number(fast_x) || vx > to_number(fast_x) ||",
      to: "       vx < -to_number(fast_y) || vx > to_number(fast_x) ||",
    },
    {
      note: "the z lower bound reads the x threshold",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "       vz < -to_number(fast_z) || vz > to_number(fast_z))) {",
      to: "       vz < -to_number(fast_x) || vz > to_number(fast_z))) {",
    },
    {
      note: "the fast_x table lookup is dropped",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), wt_fast_x(wt), Value(99.0));",
      to: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), Value(), Value(99.0));",
    },
    {
      note: "the base fast_x is ignored",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), wt_fast_x(wt), Value(99.0));",
      to: "  const Value fast_x = coalesce2(wt_fast_x(wt), field_or(base, u\"fast_vx\"), Value(99.0));",
    },
    {
      note: "the fast_x default is zero",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), wt_fast_x(wt), Value(99.0));",
      to: "  const Value fast_x = coalesce2(field_or(base, u\"fast_vx\"), wt_fast_x(wt), Value(0.0));",
    },
    {
      note: "the fast_y default is zero",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value fast_y = coalesce2(field_or(base, u\"fast_vy\"), wt_fast_y(wt), Value(99.0));",
      to: "  const Value fast_y = coalesce2(field_or(base, u\"fast_vy\"), wt_fast_y(wt), Value(0.0));",
    },
    {
      note: "the fast_z default is zero",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "  const Value fast_z = coalesce2(field_or(base, u\"fast_vz\"), wt_fast_z(wt), Value(99.0));",
      to: "  const Value fast_z = coalesce2(field_or(base, u\"fast_vz\"), wt_fast_z(wt), Value(0.0));",
    },
    {
      note: "the nullish check ignores null",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "bool nullish(const Value& v) {\n  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);\n}",
      to: "bool nullish(const Value& v) {\n  return std::holds_alternative<std::monostate>(v);\n}",
    },
    {
      note: "the align source frame is not the current one",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    const Value nf = e.find_align_frame(e.frame_id(), e.data_indexes_in_the_skys(),",
      to: "    const Value nf = e.find_align_frame(Value(), e.data_indexes_in_the_skys(),",
    },
    {
      note: "the throwing and sky index arguments are swapped",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    const Value nf = e.find_align_frame(e.frame_id(), e.data_indexes_in_the_skys(),\n                                        e.data_indexes_throwings());",
      to: "    const Value nf = e.find_align_frame(e.frame_id(), e.data_indexes_throwings(),\n                                        e.data_indexes_in_the_skys());",
    },
    {
      note: "the align frame is entered unconditionally",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "    if (truthy(nf)) {",
      to: "    if (true) {",
    },
    {
      note: "dropping is set instead of cleared",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "      e.set_dropping(false);",
      to: "      e.set_dropping(true);",
    },
    {
      note: "an undefined frame is entered",
      file: "native/lfw/state/weapon_state_misc.cpp",
      from: "      e.enter_frame(nf);",
      to: "      e.enter_frame(Value());",
    },
  ],
};
