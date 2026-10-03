/**
 * Mutation spec for `native/lfw/state/character_state_burning.{h,cpp}`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `CharacterState_Burning` has a **no-argument** constructor in the TS original
 *    (`super(StateEnum.Burning)`), so there is no default-argument mutation to make.
 * 2. `leave`'s `super.leave(e, next_frame)` resolves to `State_Base.leave`, whose
 *    `HealSelf` switch never matches `StateEnum.Burning` -- dropping the call is
 *    unobservable. (The port keeps it for structure.)
 * 3. `to_number(vx) > 0` cannot be mutated to `>= 0`: a **zero** x velocity is already
 *    filtered out by the surrounding `if (vx)` truthiness test, so the boundary is
 *    unreachable.
 * 4. `enter` calling `super.**update**(e)` (not `super.enter`) is a quirk of the TS
 *    original; the port reproduces it by calling `CharacterState_Base::update` from
 *    the installed lambda. There is no `enter` on `CharacterState_Base` to call.
 * 5. `e.catcher.drop_catching()` and `e.play_sound(...)` are entity-side actions; the
 *    catcher path is a seam (`has_catcher()` + `catcher_drop_catching()`), and
 *    `CharacterState_Burning` has no `play_sound` at all (unlike `State_Frozen`).
 */
export default {
  subject: "character_state_burning",
  mutations: [
    {
      note: "the frame on_landing is ignored",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }\n",
      to: "",
    },
    {
      note: "the index wins over the frame on_landing",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  if (!truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "an already bounced entity bounces again",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (!truthy(e.bounced())) {",
      to: "  if (truthy(e.bounced())) {",
    },
    {
      note: "the bounced flag is ignored",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (!truthy(e.bounced())) {",
      to: "  if (true) {",
    },
    {
      note: "the fall speed test is exclusive",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
      to: "    if (to_number(vy) < to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
    },
    {
      note: "the fall speed test is reversed",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
      to: "    if (to_number(vy) > to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
    },
    {
      note: "the fall threshold reads the sideways threshold",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '    if (to_number(vy) <= to_number(e.world_dataset(u"cha_bc_tst_spd_y")) ||',
      to: '    if (to_number(vy) <= to_number(e.world_dataset(u"cha_bc_tst_spd_x")) ||',
    },
    {
      note: "the two bounce conditions are combined with and",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
      to: "    if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) &&",
    },
    {
      note: "the sideways speed test is reversed",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "        abs(to_number(lfw::field_or(velocity, u\"x\"))) >\n            to_number(e.world_dataset(u\"cha_bc_tst_spd_x\"))) {",
      to: "        abs(to_number(lfw::field_or(velocity, u\"x\"))) <\n            to_number(e.world_dataset(u\"cha_bc_tst_spd_x\"))) {",
    },
    {
      note: "the sideways speed is not made positive",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "        abs(to_number(lfw::field_or(velocity, u\"x\"))) >",
      to: "        to_number(lfw::field_or(velocity, u\"x\")) >",
    },
    {
      note: "the sideways threshold reads the fall threshold",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '            to_number(e.world_dataset(u"cha_bc_tst_spd_x"))) {',
      to: '            to_number(e.world_dataset(u"cha_bc_tst_spd_y"))) {',
    },
    {
      note: "the sideways speed reads the fall speed",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '        abs(to_number(lfw::field_or(velocity, u"x"))) >',
      to: '        abs(to_number(lfw::field_or(velocity, u"y"))) >',
    },
    {
      note: "the fall speed is never read",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    const Value vy = lfw::field_or(velocity, u\"y\");\n",
      to: "    const Value vy = lfw::field_or(velocity, u\"x\");\n",
    },
    {
      note: "the bounce frame takes the first entry",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '      e.enter_frame_by_id(\n          to_string(index_by(index_by(e.data_indexes_bouncing(), u"-1"), u"1")));',
      to: '      e.enter_frame_by_id(\n          to_string(index_by(index_by(e.data_indexes_bouncing(), u"-1"), u"0")));',
    },
    {
      note: "the bounce frame is never entered",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '      e.enter_frame_by_id(\n          to_string(index_by(index_by(e.data_indexes_bouncing(), u"-1"), u"1")));\n',
      to: "",
    },
    {
      note: "the bounce velocity axes are swapped",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "      e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "      e.set_velocity(e.world_dataset(u\"cha_bc_spd\"), Value(NullTag{}), Value());",
    },
    {
      note: "the bounce velocity is not written",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "      e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "      e.set_velocity(Value(NullTag{}), Value(), Value());",
    },
    {
      note: "the bounce velocity is never applied",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "      e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());\n",
      to: "",
    },
    {
      note: "the bounced flag is not set on a bounce",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "      e.set_bounced(Value(true));\n",
      to: "",
    },
    {
      note: "the bounce leaves the entity un-bounced",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "      e.set_bounced(Value(true));",
      to: "      e.set_bounced(Value(false));",
    },
    {
      note: "the lying frame reads the up facing",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '  e.enter_frame_by_id(to_string(index_by(e.data_indexes_lying(), u"-1")));',
      to: '  e.enter_frame_by_id(to_string(index_by(e.data_indexes_lying(), u"1")));',
    },
    {
      note: "the lying frame is read from the bouncing index",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '  e.enter_frame_by_id(to_string(index_by(e.data_indexes_lying(), u"-1")));',
      to: '  e.enter_frame_by_id(to_string(index_by(e.data_indexes_bouncing(), u"-1")));',
    },
    {
      note: "the lying frame is never entered",
      file: "native/lfw/state/character_state_burning.cpp",
      from: '  e.enter_frame_by_id(to_string(index_by(e.data_indexes_lying(), u"-1")));\n',
      to: "",
    },
    {
      note: "entering does not clear the bounced flag",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "void csb_enter_body(IStateEntity& e) {\n  e.set_bounced(Value(false));",
      to: "void csb_enter_body(IStateEntity& e) {\n  e.set_bounced(Value(true));",
    },
    {
      note: "entering leaves the bounced flag alone",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "void csb_enter_body(IStateEntity& e) {\n  e.set_bounced(Value(false));\n",
      to: "void csb_enter_body(IStateEntity& e) {\n",
    },
    {
      note: "entering only drops the catcher when there is none",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (e.has_catcher()) e.catcher_drop_catching();",
      to: "  if (!e.has_catcher()) e.catcher_drop_catching();",
    },
    {
      note: "entering never drops the catcher",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (e.has_catcher()) e.catcher_drop_catching();\n",
      to: "",
    },
    {
      note: "entering does not drive the base update",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    this->CharacterState_Base::update(e);\n",
      to: "",
    },
    {
      note: "entering never resets the bounce state",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "    csb_enter_body(e);\n",
      to: "",
    },
    {
      note: "the landing hook is not installed",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  on_landing = &csb_on_landing;",
      to: "  on_landing = nullptr;",
    },
    {
      note: "burning does not drive the base update",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "void CharacterState_Burning::update(IStateEntity& e) {\n  CharacterState_Base::update(e);",
      to: "void CharacterState_Burning::update(IStateEntity& e) {",
    },
    {
      note: "the facing is derived from the z velocity",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  const Value vx = e.velocity_x();",
      to: "  const Value vx = e.velocity_z();",
    },
    {
      note: "the facing is never turned",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (truthy(vx)) e.set_facing(Value(to_number(vx) > 0 ? -1.0 : 1.0));\n",
      to: "",
    },
    {
      note: "the facing is turned the wrong way",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (truthy(vx)) e.set_facing(Value(to_number(vx) > 0 ? -1.0 : 1.0));",
      to: "  if (truthy(vx)) e.set_facing(Value(to_number(vx) > 0 ? 1.0 : -1.0));",
    },
    {
      note: "the facing test is reversed",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  if (truthy(vx)) e.set_facing(Value(to_number(vx) > 0 ? -1.0 : 1.0));",
      to: "  if (truthy(vx)) e.set_facing(Value(to_number(vx) < 0 ? -1.0 : 1.0));",
    },
    {
      note: "leaving does not clear the bounced flag",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  CharacterState_Base::leave(e, next_frame);\n  e.set_bounced(Value(false));",
      to: "  CharacterState_Base::leave(e, next_frame);\n  e.set_bounced(Value(true));",
    },
    {
      note: "leaving leaves the bounced flag alone",
      file: "native/lfw/state/character_state_burning.cpp",
      from: "  CharacterState_Base::leave(e, next_frame);\n  e.set_bounced(Value(false));\n",
      to: "  CharacterState_Base::leave(e, next_frame);\n",
    },
  ],
};
