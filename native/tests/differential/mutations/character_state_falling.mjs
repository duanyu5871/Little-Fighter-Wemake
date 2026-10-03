// Mutation spec for the `character_state_falling` differential slice.
//
// Subject: native/lfw/state/character_state_falling.{h,cpp}
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `[...indexes.bouncing[1], ...indexes.bouncing[-1]]` is a `Set`, so the spread
//    ORDER inside the cached set is unobservable. What IS observable (and mutated
//    below) is which ids end up in it.
//  * `e.data.id` / `e.frame.id` / `e.data.indexes` / `e.velocity` / `e.bounced` /
//    `e.hp` / `e.wait` / `e.shaking` / `e.facing` are property reads in the
//    original, so both harnesses keep them silent: their VALUES are what matters
//    (they are mutated through the seam overrides instead).
//  * `super.leave` (the StateEnum.HealSelf buff grant in `state/State_Base`) is
//    not re-covered here; `state_base/main` owns it. The cases all use state 12.
//  * The assignment order in `leave` is unobservable (four silent setters with
//    four distinct targets); the VALUES written are mutated below.
//  * `find_direction` itself belongs to `entity/find_frame_direction` and is
//    covered by `entity_helpers/all`; this subject only locks the `||` fallback
//    chain and the pair lookup keys.
//  * `to_number` around `vy`/`vx` mirrors JS relational comparison for numbers,
//    numeric strings and NaN; cases keep those fields numeric (a string/string
//    comparison has no JS-number analogue).

export default {
  subject: "character_state_falling",
  mutations: [
    {
      note: "the falling state drops its default state",
      file: "native/lfw/state/character_state_falling.h",
      from: "= Value(static_cast<double>(StateEnum::Falling)));",
      to: "= Value(0.0));",
    },
    {
      note: "the falling state drops its state argument",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "CharacterState_Falling::CharacterState_Falling(Value state)\n    : CharacterState_Base(std::move(state)) {",
      to: "CharacterState_Falling::CharacterState_Falling(Value state)\n    : CharacterState_Base(Value()) {",
    },
    {
      note: "the constructor never installs enter",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  enter = [this](IStateEntity& e, const Value& prev_frame) {",
      to: "  auto unused_enter = [this](IStateEntity& e, const Value& prev_frame) {",
    },
    {
      note: "the constructor never installs on_landing",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  on_landing = &csf_on_landing;",
      to: "  (void)&csf_on_landing;",
    },
    {
      note: "enter does not clear the bounced flag",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.set_bounced(Value(false));\n    e.ctrl_reset_key_list();",
      to: "    e.set_bounced(Value(true));\n    e.ctrl_reset_key_list();",
    },
    {
      note: "enter does not reset the key list",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.set_bounced(Value(false));\n    e.ctrl_reset_key_list();",
      to: "    e.set_bounced(Value(false));",
    },
    {
      note: "the bouncing cache is keyed by the frame id",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const std::u16string id = to_string(e.data_id());",
      to: "    const std::u16string id = to_string(e.frame_id());",
    },
    {
      note: "the bouncing cache is consulted by frame id",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  const auto it = _bouncing_frames_map.find(to_string(e.data_id()));",
      to: "  const auto it = _bouncing_frames_map.find(to_string(e.frame_id()));",
    },
    {
      note: "the bouncing set membership test is read from the data id",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  const Value fid = e.frame_id();\n  const std::u16string* text = as_text(fid);",
      to: "  const Value fid = e.data_id();\n  const std::u16string* text = as_text(fid);",
    },
    {
      note: "the bouncing set membership result is inverted",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  return it->second.find(*text) != it->second.end();",
      to: "  return it->second.find(*text) == it->second.end();",
    },
    {
      note: "a missing cache entry counts as a bouncing frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (it == _bouncing_frames_map.end()) return false;",
      to: "  if (it == _bouncing_frames_map.end()) return true;",
    },
    {
      note: "the bouncing set is rebuilt on every enter",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    bool need_cache = _bouncing_frames_map.find(id) == _bouncing_frames_map.end();",
      to: "    bool need_cache = true;",
    },
    {
      note: "the bouncing set is never cached",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    bool need_cache = _bouncing_frames_map.find(id) == _bouncing_frames_map.end();",
      to: "    bool need_cache = _bouncing_frames_map.find(id) != _bouncing_frames_map.end();",
    },
    {
      note: "a missing bouncing pair still caches an empty set",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "      need_cache = truthy(bouncing);",
      to: "      need_cache = true;",
    },
    {
      note: "the cached set keeps the old data id",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        _bouncing_frames_map[id] = std::move(frames);",
      to: "        _bouncing_frames_map.clear();\n        _bouncing_frames_map[id] = std::move(frames);",
    },
    {
      note: "the cached set is left empty",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        _bouncing_frames_map[id] = std::move(frames);",
      to: "        _bouncing_frames_map[id] = std::set<std::u16string>();",
    },
    {
      note: "only the first id of each pair is cached",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "          for (size_t i = 0; i < a->size(); ++i) frames.insert(to_string(a->at(i)));",
      to: "          for (size_t i = 0; i < 1; ++i) frames.insert(to_string(a->at(i)));",
    },
    {
      note: "only the first direction is cached",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        const Value minus = js_at(bouncing, Value(-1.0));",
      to: "        const Value minus = js_at(bouncing, Value(1.0));",
    },
    {
      note: "the cached ids keep their raw form",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "          for (size_t i = 0; i < b->size(); ++i) frames.insert(to_string(b->at(i)));",
      to: "          for (size_t i = 0; i < b->size(); ++i) frames.insert(std::u16string());",
    },
    {
      note: "the catcher is dropped even when there is none",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (e.has_catcher()) e.catcher_drop_catching();",
      to: "    e.catcher_drop_catching();",
    },
    {
      note: "the catcher is never released",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (e.has_catcher()) e.catcher_drop_catching();\n    e.drop_holding();",
      to: "    e.drop_holding();",
    },
    {
      note: "the held object is never dropped",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.drop_holding();\n\n    if (to_number(e.hp()) <= 0) {",
      to: "\n    if (to_number(e.hp()) <= 0) {",
    },
    {
      note: "the fusion gate ignores hp zero",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (to_number(e.hp()) <= 0) {",
      to: "    if (to_number(e.hp()) < 0) {",
    },
    {
      note: "an empty fusion still dismisses",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "      if (value_length(fuse_bys) > 0) {",
      to: "      if (value_length(fuse_bys) >= 0) {",
    },
    {
      note: "the fused velocity keeps the forward speed",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "          next_vx = next_vx * -1.0;",
      to: "          next_vx = next_vx * 1.0;",
    },
    {
      note: "the fused velocity reads the depth speed",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        const Value vx = e.velocity_x();\n        const double vy = e.velocity_y();",
      to: "        const Value vx = e.velocity_z();\n        const double vy = e.velocity_y();",
    },
    {
      note: "the fused velocity passes the forward speed as depth",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        const Value vz = e.velocity_z();\n        double next_vx = to_number(vx);",
      to: "        const Value vz = e.velocity_x();\n        double next_vx = to_number(vx);",
    },
    {
      note: "the fused y and z speeds are swapped",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "          e.ref_set_velocity(arr->at(i), Value(next_vx), Value(vy), Value(vz));",
      to: "          e.ref_set_velocity(arr->at(i), Value(next_vx), Value(vz), Value(vy));",
    },
    {
      note: "the fusion is dismissed with a constant frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        e.dismiss_fusion(e.frame_id());",
      to: "        e.dismiss_fusion(Value(u\"\"));",
    },
    {
      note: "the fusion is never dismissed",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "        e.dismiss_fusion(e.frame_id());\n      }\n    }\n    e.leave_ground();",
      to: "      }\n    }\n    e.leave_ground();",
    },
    {
      note: "the ground is never left",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.leave_ground();\n  };",
      to: "  };",
    },
    {
      note: "shaking no longer stops the update",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (to_number(e.shaking()) > 0) return;",
      to: "  if (to_number(e.shaking()) > 1) return;",
    },
    {
      note: "a non positive shake value stops the update",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (to_number(e.shaking()) > 0) return;",
      to: "  if (to_number(e.shaking()) >= 0) return;",
    },
    {
      note: "the bouncing and falling branches are swapped",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (is_bouncing_frame(e)) {\n    update_bouncing(e);\n  } else {\n    update_falling(e);\n  }",
      to: "  if (is_bouncing_frame(e)) {\n    update_falling(e);\n  } else {\n    update_bouncing(e);\n  }",
    },
    {
      note: "the bouncing decay factor is wrong",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.handle_ground_velocity_decay(0.7);",
      to: "  e.handle_ground_velocity_decay(0.8);",
    },
    {
      note: "the falling frame is chosen even when wait is positive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (to_number(e.wait()) <= 0) {",
      to: "  if (to_number(e.wait()) < 0) {",
    },
    {
      note: "the vy window starts on the first frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    double falling_frame_idx = 1;",
      to: "    double falling_frame_idx = 0;",
    },
    {
      note: "the rising vy test is inclusive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (vy > 3) falling_frame_idx = 0;",
      to: "    if (vy >= 3) falling_frame_idx = 0;",
    },
    {
      note: "the rising vy picks the last frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (vy > 3) falling_frame_idx = 0;",
      to: "    if (vy > 3) falling_frame_idx = 2;",
    },
    {
      note: "the falling vy test is inclusive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (vy < -3) falling_frame_idx = 2;",
      to: "    if (vy <= -3) falling_frame_idx = 2;",
    },
    {
      note: "the falling vy picks the first frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    if (vy < -3) falling_frame_idx = 2;",
      to: "    if (vy < -3) falling_frame_idx = 0;",
    },
    {
      note: "the direction ignores the facing",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? 1 : -1;",
      to: "    const double direction = to_number(vx) > 0 ? 1 : -1;",
    },
    {
      note: "a zero speed counts as positive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? 1 : -1;",
      to: "    const double direction = to_number(vx) / to_number(e.facing()) >= 0 ? 1 : -1;",
    },
    {
      note: "the direction branches are swapped",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? 1 : -1;",
      to: "    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? -1 : 1;",
    },
    {
      note: "the direction subtracts instead of dividing",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const double direction = to_number(vx) / to_number(e.facing()) > 0 ? 1 : -1;",
      to: "    const double direction = to_number(vx) - to_number(e.facing()) > 0 ? 1 : -1;",
    },
    {
      note: "the falling pair ignores the direction",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "              js_at_index(js_at(e.data_indexes_falling(), Value(direction)), falling_frame_idx));",
      to: "              js_at_index(js_at(e.data_indexes_falling(), Value(1.0)), falling_frame_idx));",
    },
    {
      note: "the falling frame index is a constant",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "              js_at_index(js_at(e.data_indexes_falling(), Value(direction)), falling_frame_idx));",
      to: "              js_at_index(js_at(e.data_indexes_falling(), Value(direction)), 1.0));",
    },
    {
      note: "the falling frame is built from the wrong key",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    frame.set(u\"id\",",
      to: "    frame.set(u\"fid\",",
    },
    {
      note: "the falling frame is never entered",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame(Value(std::make_shared<Object>(frame)));",
      to: "    (void)frame;",
    },
    {
      note: "landing ignores the on_landing frame",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  const Value landing = e.frame_on_landing();",
      to: "  const Value landing = e.frame_id();",
    },
    {
      note: "the on_landing fame test is inverted",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);",
      to: "  if (!truthy(landing)) {\n    e.enter_frame(landing);",
    },
    {
      note: "the on_landing frame does not end the landing",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame(landing);\n    return;\n  }",
      to: "    e.enter_frame(landing);\n  }",
    },
    {
      note: "the bouncing pair is read from the lying pair",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  double d = entity::find_direction(frame, e.data_indexes_bouncing());",
      to: "  double d = entity::find_direction(frame, e.data_indexes_lying());",
    },
    {
      note: "the falling pair is never consulted",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (!d) d = entity::find_direction(frame, e.data_indexes_falling());",
      to: "  if (false) d = entity::find_direction(frame, e.data_indexes_falling());",
    },
    {
      note: "the critical pair is never consulted",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (!d) d = entity::find_direction(frame, e.data_indexes_critical_hit());",
      to: "  if (false) d = entity::find_direction(frame, e.data_indexes_critical_hit());",
    },
    {
      note: "the facing is never used as the last fallback",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (!d) d = to_number(facing);",
      to: "  (void)facing;",
    },
    {
      note: "a found direction is overwritten by the next pair",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (!d) d = entity::find_direction(frame, e.data_indexes_falling());",
      to: "  if (d) d = entity::find_direction(frame, e.data_indexes_falling());",
    },
    {
      note: "the landing speeds are read swapped",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  const Value vy = lfw::field_or(velocity, u\"y\");\n  const Value vx = lfw::field_or(velocity, u\"x\");",
      to: "  const Value vy = lfw::field_or(velocity, u\"x\");\n  const Value vx = lfw::field_or(velocity, u\"y\");",
    },
    {
      note: "the y speed test is exclusive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "      (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
      to: "      (to_number(vy) < to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) ||",
    },
    {
      note: "the x speed test drops the absolute value",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "       abs(to_number(vx)) > to_number(e.world_dataset(u\"cha_bc_tst_spd_x\")))) {",
      to: "       to_number(vx) > to_number(e.world_dataset(u\"cha_bc_tst_spd_x\")))) {",
    },
    {
      note: "the x speed test is inclusive",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "       abs(to_number(vx)) > to_number(e.world_dataset(u\"cha_bc_tst_spd_x\")))) {",
      to: "       abs(to_number(vx)) >= to_number(e.world_dataset(u\"cha_bc_tst_spd_x\")))) {",
    },
    {
      note: "an already bounced entity bounces again",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  if (!truthy(e.bounced()) &&",
      to: "  if (truthy(e.bounced()) &&",
    },
    {
      note: "the bouncing frame reads the wrong pair slot",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame_by_id(to_string(js_at_index(js_at(bouncing, Value(d)), 1)));",
      to: "    e.enter_frame_by_id(to_string(js_at_index(js_at(bouncing, Value(d)), 0)));",
    },
    {
      note: "the bouncing frame reads the opposite direction",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame_by_id(to_string(js_at_index(js_at(bouncing, Value(d)), 1)));",
      to: "    e.enter_frame_by_id(to_string(js_at_index(js_at(bouncing, Value(-d)), 1)));",
    },
    {
      note: "the bounce does not reset the z speed",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "    e.set_velocity(Value(NullTag{}), Value(), Value());",
    },
    {
      note: "the bounce writes the z speed into x",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "    e.set_velocity(e.world_dataset(u\"cha_bc_spd\"), Value(NullTag{}), Value());",
    },
    {
      note: "the bounce does not set the bounced flag",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.set_bounced(Value(true));",
      to: "    e.set_bounced(Value(false));",
    },
    {
      note: "the lying frame is read from the bouncing pair",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    const Value lying = e.data_indexes_lying();",
      to: "    const Value lying = e.data_indexes_bouncing();",
    },
    {
      note: "the lying frame reads the opposite direction",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame_by_id(to_string(js_at(lying, Value(d))));",
      to: "    e.enter_frame_by_id(to_string(js_at(lying, Value(-d))));",
    },
    {
      note: "the lying frame ignores the direction",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "    e.enter_frame_by_id(to_string(js_at(lying, Value(d))));",
      to: "    e.enter_frame_by_id(to_string(js_at(lying, Value(1.0))));",
    },
    {
      note: "leave does not clear the bounced flag",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_bounced(Value(false));\n  e.set_fall_value(e.fall_value_max());",
      to: "  e.set_bounced(Value(true));\n  e.set_fall_value(e.fall_value_max());",
    },
    {
      note: "leave does not restore the fall value",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_fall_value(e.fall_value_max());",
      to: "  e.set_fall_value(e.defend_value_max());",
    },
    {
      note: "leave restores the fall value from the defend maximum",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_fall_value(e.fall_value_max());\n  e.set_defend_value(e.defend_value_max());",
      to: "  e.set_defend_value(e.fall_value_max());\n  e.set_fall_value(e.defend_value_max());",
    },
    {
      note: "leave zeroes the defend value",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_defend_value(e.defend_value_max());",
      to: "  e.set_defend_value(Value(0.0));",
    },
    {
      note: "leave zeroes the resting value",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_resting(e.resting_max());",
      to: "  e.set_resting(Value(0.0));",
    },
    {
      note: "leave does not clear the fall injury",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_fallinjury(Value(0.0));",
      to: "  e.set_fallinjury(Value(1.0));",
    },
    {
      note: "leave does not clear the throw injury",
      file: "native/lfw/state/character_state_falling.cpp",
      from: "  e.set_throwinjury(Value(0.0));",
      to: "  e.set_throwinjury(Value(1.0));",
    },
  ],
};
