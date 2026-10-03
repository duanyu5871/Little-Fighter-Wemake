// Mutation spec for the `state_base_proxy` differential slice.
//
// Subject: native/lfw/state/state_base_proxy.{h,cpp} (StateBase_Proxy + State_15 + State_Frozen)
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `pre_update` and `on_dead` are forwarded by the proxy, but NO proxy target
//    installs them -- `State_Base`, `CharacterState_Base`, `WeaponState_Base` and
//    `BallState_Base` all leave them empty -- so "call them twice" / "never call
//    them" cannot be observed. Same for `get_gravity` and `find_frame_by_id`
//    (no target installs either).
//  * `leave` and `on_restrict` are virtual methods on `State_Base`; every proxy
//    target uses the SAME base implementation, so forwarding to `_proxy` instead
//    of the dispatched one, or dropping/normalising the arguments, is unobservable
//    here (mutations of the base body itself belong to the `state_base` slice).
//    Note that `State_Frozen::leave` DOES keep its own override, so the double
//    `StateBase_Proxy::leave` call IS observable through the heal self grant.
//  * `set_position` is silent on both harnesses because the original writes
//    `e.position.x/y/z` (three plain property writes) while the port calls
//    `set_position(x, y, z)` once; the position is compared through the state
//    text instead.
//  * `e.hp -= 10` is a plain property write in the original, so the port's
//    `set_hp` seam is silent and the value is compared through the state text.

export default {
  subject: "state_base_proxy",
  mutations: [
    {
      note: "the fighter branch classifies weapons",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (entity::is_fighter_data(data)) return _character_proxy;",
      to: "  if (entity::is_weapon_data(data)) return _character_proxy;",
    },
    {
      note: "the fighter branch is dropped",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (entity::is_fighter_data(data)) return _character_proxy;\n  if (entity::is_weapon_data(data)) return _weapon_proxy;",
      to: "  if (entity::is_weapon_data(data)) return _weapon_proxy;",
    },
    {
      note: "the weapon and ball branches are swapped",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (entity::is_weapon_data(data)) return _weapon_proxy;\n  if (entity::is_ball_data(data)) return _ball_proxy;",
      to: "  if (entity::is_ball_data(data)) return _weapon_proxy;\n  if (entity::is_weapon_data(data)) return _ball_proxy;",
    },
    {
      note: "the ball branch is dropped",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (entity::is_ball_data(data)) return _ball_proxy;\n  return _proxy;",
      to: "  return _proxy;",
    },
    {
      note: "everything falls back to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (entity::is_fighter_data(data)) return _character_proxy;",
      to: "  if (entity::is_fighter_data(data)) return _proxy;",
    },
    {
      note: "the entity data is not read",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  const Value data = e.data();",
      to: "  const Value data = Value();",
    },
    {
      note: "enter is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    State_Base& p = get_proxy(e);\n    if (p.enter) p.enter(e, prev_frame);",
      to: "    State_Base& p = get_proxy(e);\n    (void)p;",
    },
    {
      note: "enter is forwarded to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.enter) p.enter(e, prev_frame);",
      to: "    if (_proxy.enter) _proxy.enter(e, prev_frame);",
    },
    {
      note: "enter is called twice",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.enter) p.enter(e, prev_frame);",
      to: "    if (p.enter) { p.enter(e, prev_frame); p.enter(e, prev_frame); }",
    },
    {
      note: "on_landing is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    State_Base& p = get_proxy(e);\n    if (p.on_landing) p.on_landing(e, velocity);",
      to: "    State_Base& p = get_proxy(e);\n    (void)p;",
    },
    {
      note: "on_landing is forwarded to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.on_landing) p.on_landing(e, velocity);",
      to: "    if (_proxy.on_landing) _proxy.on_landing(e, velocity);",
    },
    {
      note: "get_gravity ignores the target",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_gravity) return p.get_gravity(e);\n    return Value();",
      to: "    (void)p;\n    return Value(1.0);",
    },
    {

      note: "get_auto_frame ignores the target",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_auto_frame) return p.get_auto_frame(e);\n    return Value();",
      to: "    (void)p;\n    return Value(u\"X\");",
    },
    {
      note: "get_auto_frame is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_auto_frame) return p.get_auto_frame(e);",
      to: "    if (false) return p.get_auto_frame(e);",
    },
    {
      note: "get_auto_frame is forwarded to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_auto_frame) return p.get_auto_frame(e);",
      to: "    if (_proxy.get_auto_frame) return _proxy.get_auto_frame(e);",
    },
    {
      note: "get_sudden_death_frame is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_sudden_death_frame) return p.get_sudden_death_frame(e);",
      to: "    if (false) return p.get_sudden_death_frame(e);",
    },
    {
      note: "get_sudden_death_frame ignores the target",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_sudden_death_frame) return p.get_sudden_death_frame(e);\n    return Value();",
      to: "    (void)p;\n    return Value(u\"X\");",
    },
    {
      note: "get_sudden_death_frame is forwarded to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_sudden_death_frame) return p.get_sudden_death_frame(e);",
      to: "    if (_proxy.get_sudden_death_frame) return _proxy.get_sudden_death_frame(e);",
    },
    {
      note: "get_caught_end_frame is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_caught_end_frame) return p.get_caught_end_frame(e);",
      to: "    if (false) return p.get_caught_end_frame(e);",
    },
    {
      note: "get_caught_end_frame ignores the target",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.get_caught_end_frame) return p.get_caught_end_frame(e);\n    return Value();",
      to: "    (void)p;\n    return Value(u\"X\");",
    },
    {
      note: "find_frame_by_id ignores the target",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.find_frame_by_id) return p.find_frame_by_id(e, id);\n    return Value();",
      to: "    (void)p;\n    (void)id;\n    return Value(u\"X\");",
    },
    {
      note: "on_leave_ground is never forwarded",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    State_Base& p = get_proxy(e);\n    if (p.on_leave_ground) p.on_leave_ground(e);",
      to: "    State_Base& p = get_proxy(e);\n    (void)p;",
    },
    {
      note: "on_leave_ground is forwarded to the plain proxy",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (p.on_leave_ground) p.on_leave_ground(e);",
      to: "    if (_proxy.on_leave_ground) _proxy.on_leave_ground(e);",
    },
    {
      note: "update is not dispatched",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "void StateBase_Proxy::update(IStateEntity& e) { get_proxy(e).update(e); }",
      to: "void StateBase_Proxy::update(IStateEntity& e) { (void)e; }",
    },
    {
      note: "update is dispatched to the plain state",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "void StateBase_Proxy::update(IStateEntity& e) { get_proxy(e).update(e); }",
      to: "void StateBase_Proxy::update(IStateEntity& e) { _proxy.update(e); }",
    },
    {
      note: "State_15 does not use the Normal state",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "State_15::State_15() : StateBase_Proxy(Value(static_cast<double>(StateEnum::Normal))) {}",
      to: "State_15::State_15() : StateBase_Proxy(Value(static_cast<double>(StateEnum::Standing))) {}",
    },
    {
      note: "frozen enter ignores the catcher",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (e.has_catcher()) e.catcher_drop_catching();",
      to: "    if (false) e.catcher_drop_catching();",
    },
    {
      note: "frozen enter drops the holding in every case",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n      e.drop_holding();\n    }",
      to: "    e.drop_holding();",
    },
    {
      note: "frozen enter never drops the holding",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n      e.drop_holding();\n    }",
      to: "    if (false) {\n      e.drop_holding();\n    }",
    },
    {
      note: "the heavy holding test is strict",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "    if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "frozen enter does not play a sound",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.play_sound(sound_list(u\"data/065.wav.mp3\"));",
      to: "    (void)0;",
    },
    {
      note: "frozen enter plays the leave sound",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.play_sound(sound_list(u\"data/065.wav.mp3\"));",
      to: "    e.play_sound(sound_list(u\"data/066.wav.mp3\"));",
    },
    {
      note: "frozen enter skips the parent",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    if (super_enter) super_enter(e, prev_frame);",
      to: "    (void)super_enter;",
    },
    {
      note: "frozen enter never installs the wrapper",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  const auto super_enter = enter;",
      to: "  const std::function<void(IStateEntity&, const Value&)> super_enter = nullptr;",
    },
    {
      note: "frozen leave skips the parent",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  StateBase_Proxy::leave(e, next_frame);\n  e.play_sound(sound_list(u\"data/066.wav.mp3\"));",
      to: "  e.play_sound(sound_list(u\"data/066.wav.mp3\"));",
    },
    {
      note: "frozen leave does not play the leave sound",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  e.play_sound(sound_list(u\"data/066.wav.mp3\"));\n  e.apply_opoints(ice_piece_opoints());",
      to: "  e.apply_opoints(ice_piece_opoints());",
    },
    {
      note: "frozen leave does not apply the opoints",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  e.apply_opoints(ice_piece_opoints());\n  StateBase_Proxy::leave(e, next_frame);",
      to: "  StateBase_Proxy::leave(e, next_frame);",
    },
    {
      note: "frozen leave does not call the parent twice",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  e.apply_opoints(ice_piece_opoints());\n  StateBase_Proxy::leave(e, next_frame);",
      to: "  e.apply_opoints(ice_piece_opoints());",
    },
    {
      note: "frozen leave does not call the parent at all",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "void State_Frozen::leave(IStateEntity& e, const Value& next_frame) {\n  StateBase_Proxy::leave(e, next_frame);",
      to: "void State_Frozen::leave(IStateEntity& e, const Value& next_frame) {\n  (void)next_frame;",
    },
    {
      note: "frozen landing ignores the frame",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  const Value landing = e.frame_on_landing();\n  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  const Value landing = e.frame_on_landing();\n  if (false) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "frozen landing always uses the frame",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  const Value landing = e.frame_on_landing();\n  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  const Value landing = e.frame_on_landing();\n  if (true) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "frozen landing reads the velocity x",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  const Value vy = field_or(velocity, u\"y\");",
      to: "  const Value vy = field_or(velocity, u\"x\");",
    },
    {
      note: "the ground speed threshold is not doubled",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) * 2) {",
      to: "  if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\"))) {",
    },
    {
      note: "the ground speed threshold is strictly smaller",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) * 2) {",
      to: "  if (to_number(vy) < to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) * 2) {",
    },
    {
      note: "the ground speed threshold is always met",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) * 2) {",
      to: "  if (true) {",
    },
    {
      note: "the ground speed threshold is never met",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  if (to_number(vy) <= to_number(e.world_dataset(u\"cha_bc_tst_spd_y\")) * 2) {",
      to: "  if (false) {",
    },
    {
      note: "the bouncing index key is wrong",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(index_by(e.data_indexes_bouncing(), u\"-1\"))));",
      to: "    e.enter_frame_by_id(to_string(index_0(index_by(e.data_indexes_bouncing(), u\"1\"))));",
    },
    {
      note: "the bouncing index is not taken from the list",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(index_by(e.data_indexes_bouncing(), u\"-1\"))));",
      to: "    e.enter_frame_by_id(to_string(index_by(e.data_indexes_bouncing(), u\"-1\")));",
    },
    {
      note: "the frozen bounce does not reset the velocity",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "    e.set_velocity(Value(0.0), e.world_dataset(u\"cha_bc_spd\"), Value());",
    },
    {
      note: "the frozen bounce does not apply the spark speed",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.set_velocity(Value(NullTag{}), e.world_dataset(u\"cha_bc_spd\"), Value());",
      to: "    e.set_velocity(Value(NullTag{}), Value(0.0), Value());",
    },
    {
      note: "the frozen bounce does not hurt",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.set_hp(Value(to_number(e.hp()) - 10));",
      to: "    e.set_hp(Value(to_number(e.hp())));",
    },
    {
      note: "the frozen bounce hurts by the wrong amount",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "    e.set_hp(Value(to_number(e.hp()) - 10));",
      to: "    e.set_hp(Value(to_number(e.hp()) - 1));",
    },
    {
      note: "the state frozen default argument is wrong",
      file: "native/lfw/state/state_base_proxy.h",
      from: "  explicit State_Frozen(Value state = Value(static_cast<double>(StateEnum::Frozen)));",
      to: "  explicit State_Frozen(Value state = Value(static_cast<double>(StateEnum::Burning)));",
    },
    {
      note: "the state frozen default argument is missing",
      file: "native/lfw/state/state_base_proxy.h",
      from: "  explicit State_Frozen(Value state = Value(static_cast<double>(StateEnum::Frozen)));",
      to: "  explicit State_Frozen(Value state = Value(0.0));",
    },
    {
      note: "the sound wrapper builds an empty list",
      file: "native/lfw/state/state_base_proxy.cpp",
      from: "  arr->push_back(Value(std::u16string(path)));",
      to: "  (void)path;",
    },
  ],
};
