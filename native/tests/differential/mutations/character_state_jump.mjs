// Mutation spec for the `character_state_jump` differential slice.
//
// Subject: native/lfw/state/character_state_jump.{h,cpp}
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * The bot branch runs `add_jumping_t` then `add_jumping_y`; both add the same
//    `atom_time`, so SWAPPING the two calls is unobservable and intentionally
//    absent. Skipping one of them is observable and is in the list.
//  * `is_end(key)` is `true` when the key is NOT held; each of the five key
//    reads is identified by its own log line, so cross-wiring any two reads is
//    caught by the log even when the numeric outcome coincides.
//  * `e.ground_y` / `e.jumping.*` are property reads in the original (silent);
//    the port routes them through seams that stay silent too, so only the
//    values they return are mutable.
//  * `e.position.y` reads are silent; `positions` are observed through the
//    state text.
//  * `jump_flag` truthiness: the string "0" case locks the `truthy` coercion.

export default {
  subject: "character_state_jump",
  mutations: [
    {
      note: "the jump state drops its default state",
      file: "native/lfw/state/character_state_jump.h",
      from: "= Value(static_cast<double>(StateEnum::Jump)))",
      to: "= Value(0.0))",
    },
    {
      note: "the jump state drops its state argument",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "CharacterState_Jump::CharacterState_Jump(Value state) : CharacterState_Base(std::move(state)) {",
      to: "CharacterState_Jump::CharacterState_Jump(Value state) : CharacterState_Base(Value()) {",
    },
    {
      note: "the constructor never installs enter",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  enter = &csj_enter;\n  on_landing = &csj_on_landing;",
      to: "  on_landing = &csj_on_landing;",
    },
    {
      note: "the constructor never installs on_landing",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  enter = &csj_enter;\n  on_landing = &csj_on_landing;",
      to: "  enter = &csj_enter;",
    },
    {
      note: "enter does not zero jumping_x",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.set_jumping_x(Value(0.0));",
      to: "  e.set_jumping_x(Value(1.0));",
    },
    {
      note: "enter does not zero jumping_t",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.set_jumping_t(Value(0.0));",
      to: "  e.set_jumping_t(Value(1.0));",
    },
    {
      note: "enter does not zero jumping_y",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.set_jumping_y(Value(0.0));\n  e.set_jumping_z(Value(0.0));",
      to: "  e.set_jumping_z(Value(0.0));",
    },
    {
      note: "the update drops the ground decay",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.handle_ground_velocity_decay();\n  double px = 0;",
      to: "  double px = 0;",
    },
    {
      note: "the update never returns early",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (!float_equal(py, to_number(e.ground_y()))) return;",
      to: "  if (false) return;",
    },
    {
      note: "the update returns early on the ground",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (!float_equal(py, to_number(e.ground_y()))) return;",
      to: "  if (float_equal(py, to_number(e.ground_y()))) return;",
    },
    {
      note: "the ground check reads the x position",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (!float_equal(py, to_number(e.ground_y()))) return;",
      to: "  if (!float_equal(px, to_number(e.ground_y()))) return;",
    },
    {
      note: "the prev frame is not read",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value prev = e.prev_frame();\n  const Value jump_flag = field_or(prev, u\"jump_flag\");",
      to: "  const Value prev = Value();\n  const Value jump_flag = field_or(prev, u\"jump_flag\");",
    },
    {
      note: "the jump flag is read from the wrong key",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value jump_flag = field_or(prev, u\"jump_flag\");",
      to: "  const Value jump_flag = field_or(prev, u\"jumpflag\");",
    },
    {
      note: "the launch runs even without the jump flag",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (!truthy(jump_flag)) return;",
      to: "  if (false) return;",
    },
    {
      note: "the launch never runs",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (!truthy(jump_flag)) return;",
      to: "  if (true) return;",
    },
    {
      note: "the bot check is inverted",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (e.ctrl_is_bot()) {",
      to: "  if (!e.ctrl_is_bot()) {",
    },
    {
      note: "the bot branch drops the y advance",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    add_jumping_t(e);\n    add_jumping_y(e);",
      to: "    add_jumping_t(e);",
    },
    {
      note: "the R key is read as L",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kR))) {",
      to: "    if (!e.ctrl_is_end(std::u16string(gk::kL))) {",
    },
    {
      note: "the L key is read as R",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kL))) {",
      to: "    if (!e.ctrl_is_end(std::u16string(gk::kR))) {",
    },
    {
      note: "the U key is read as D",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kU))) {",
      to: "    if (!e.ctrl_is_end(std::u16string(gk::kD))) {",
    },
    {
      note: "the D key is read as U",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kD))) {",
      to: "    if (!e.ctrl_is_end(std::u16string(gk::kU))) {",
    },
    {
      note: "the j key is read as d",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kj))) add_jumping_y(e);",
      to: "    if (!e.ctrl_is_end(std::u16string(gk::kd))) add_jumping_y(e);",
    },
    {
      note: "the j key guard is dropped",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    if (!e.ctrl_is_end(std::u16string(gk::kj))) add_jumping_y(e);",
      to: "    add_jumping_y(e);",
    },
    {
      note: "the +x advance is subtracted",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "      const Value current = e.jumping_x();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_x(Value(round_float(to_number(current) + to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kL))) {",
      to: "      const Value current = e.jumping_x();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_x(Value(round_float(to_number(current) - to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kL))) {",
    },
    {
      note: "the -x advance is added",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "      const Value current = e.jumping_x();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_x(Value(round_float(to_number(current) - to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kU))) {",
      to: "      const Value current = e.jumping_x();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_x(Value(round_float(to_number(current) + to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kU))) {",
    },
    {
      note: "the -z advance is added",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "      const Value current = e.jumping_z();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_z(Value(round_float(to_number(current) - to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kD))) {",
      to: "      const Value current = e.jumping_z();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_z(Value(round_float(to_number(current) + to_number(atom))));\n    }\n    if (!e.ctrl_is_end(std::u16string(gk::kD))) {",
    },
    {
      note: "the +z advance is subtracted",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "      const Value current = e.jumping_z();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_z(Value(round_float(to_number(current) + to_number(atom))));",
      to: "      const Value current = e.jumping_z();\n      const Value atom = e.world_dataset(u\"atom_time\");\n      e.set_jumping_z(Value(round_float(to_number(current) - to_number(atom))));",
    },
    {
      note: "the lateral input is read from the vertical axis",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double lr = e.ctrl_lr();",
      to: "  const double lr = e.ctrl_ud();",
    },
    {
      note: "the vertical input is read from the lateral axis",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double ud = e.ctrl_ud();",
      to: "  const double ud = e.ctrl_lr();",
    },
    {
      note: "the height factor reads the x factor",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value jump_h_f = e.dataset(u\"jump_h_f\");",
      to: "  const Value jump_h_f = e.dataset(u\"jump_x_f\");",
    },
    {
      note: "the depth factor reads the distance",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value jump_z_f = e.dataset(u\"jump_z_f\");",
      to: "  const Value jump_z_f = e.dataset(u\"jump_distancez\");",
    },
    {
      note: "the x factor reads the height factor",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value jump_x_f = e.dataset(u\"jump_x_f\");",
      to: "  const Value jump_x_f = e.dataset(u\"jump_h_f\");",
    },
    {
      note: "the depth drag multiplies instead of dividing",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz / 4.0));",
      to: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz * 4.0));",
    },
    {
      note: "the depth drag divides by two",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz / 4.0));",
      to: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz / 2.0));",
    },
    {
      note: "the depth drag drops the absolute value",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz / 4.0));",
      to: "  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - vz / 4.0);",
    },
    {
      note: "the minimum launch speed is wrong",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const double min_v = 4.0;",
      to: "  const double min_v = 5.0;",
    },
    {
      note: "the interpolation condition reads y",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  if (truthy(e.jumping_t())) {",
      to: "  if (truthy(e.jumping_y())) {",
    },
    {
      note: "the interpolation subtracts the minimum",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    vy = min_v + (vy - min_v) * to_number(jy) / to_number(jt);",
      to: "    vy = min_v - (vy - min_v) * to_number(jy) / to_number(jt);",
    },
    {
      note: "the interpolation adds the minimum",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    vy = min_v + (vy - min_v) * to_number(jy) / to_number(jt);",
      to: "    vy = min_v + (vy + min_v) * to_number(jy) / to_number(jt);",
    },
    {
      note: "the interpolation swaps y and t",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    vy = min_v + (vy - min_v) * to_number(jy) / to_number(jt);",
      to: "    vy = min_v + (vy - min_v) * to_number(jt) / to_number(jy);",
    },
    {
      note: "the interpolation multiplies the minimum",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "    vy = min_v + (vy - min_v) * to_number(jy) / to_number(jt);",
      to: "    vy = min_v * (vy - min_v) * to_number(jy) / to_number(jt);",
    },
    {
      note: "the launch velocity components are swapped",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.set_velocity(Value(vx), Value(vy), Value(vz));",
      to: "  e.set_velocity(Value(vz), Value(vy), Value(vx));",
    },
    {
      note: "the landing frame test is inverted",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  const Value landing = e.frame_on_landing();\n  if (truthy(landing)) {",
      to: "  const Value landing = e.frame_on_landing();\n  if (!truthy(landing)) {",
    },
    {
      note: "the landing fallback reads the second landing index",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_1()));",
      to: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_2()));",
    },
    {
      note: "the landing velocity is undefined",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  e.update_velocity(Value(std::make_shared<Object>(v)));",
      to: "  e.update_velocity(Value());",
    },
    {
      note: "the landing velocity dz is wrong",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  v.set(u\"dvz\", Value(4.0));",
      to: "  v.set(u\"dvz\", Value(5.0));",
    },
    {
      note: "the landing velocity control mode is wrong",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  v.set(u\"ctrl_z\", Value(static_cast<double>(SpeedCtrl::Control)));",
      to: "  v.set(u\"ctrl_z\", Value(static_cast<double>(SpeedCtrl::None)));",
    },
    {
      note: "the landing velocity key is misspelled",
      file: "native/lfw/state/character_state_jump.cpp",
      from: "  v.set(u\"ctrl_z\", Value(static_cast<double>(SpeedCtrl::Control)));",
      to: "  v.set(u\"ctrlz\", Value(static_cast<double>(SpeedCtrl::Control)));",
    },
  ],
};
