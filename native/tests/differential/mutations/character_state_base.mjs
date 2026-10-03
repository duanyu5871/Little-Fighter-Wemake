/**
 * Mutation spec for `native/lfw/state/character_state_base.{h,cpp}`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `State_Base::update(e)` is empty, so dropping the `super.update(e)` call in
 *    `CharacterState_Base::update` cannot be observed.
 * 2. `if (!truthy(fid)) return Value();` cannot be dropped: the only way `fid` is
 *    falsy in the case is a *missing* index, and `to_string(undefined)` is `""`,
 *    which is not a key of `frames` either -- both paths return `undefined`.
 * 3. `index_by`'s **array** branch is dead here: it is only ever called with
 *    `data.frames` (an object) or with `falling`'s `"1"` / `"-1"` entries (arrays,
 *    reached through the second `index_by` call). `index_0`'s **object** branch is
 *    likewise dead (`in_the_skys` is always an array).
 * 4. When `falling` is present but lacks the requested key, the TS original throws
 *    (`falling["1"]` is `undefined`, then `undefined[1]` is a TypeError); the port
 *    returns an empty frame instead. The case always supplies both keys.
 * 5. `e.enter_frame_by_id(to_string(e.data_indexes_landing_2()))` hands `""` to
 *    `enter_frame_by_id` where the TS original hands `undefined`; the case always
 *    supplies `landing_2`.
 * 6. The `on_landing` velocity parameter is unused by the TS original.
 * 7. `CharacterState_Base` never reads the state object's own `_state` (only
 *    `State_Base::leave` / `on_restrict` do, and this unit's case calls neither), so
 *    "the constructor does not forward the state" is unobservable here. The
 *    `state_base` spec covers the storage itself.
 */
export default {
  subject: "character_state_base",
  mutations: [
    {
      note: "index_by ignores the found entry",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    const Value* p = o->get(k);\n    if (p != nullptr) return *p;",
      to: "    const Value* p = o->get(k);\n    if (p != nullptr) return Value();",
    },
    {
      note: "the frame list takes the second entry",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() == 0) return Value();\n    return arr->at(1);",
    },
    {
      note: "index_0 rejects every non empty list",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() > 0) return Value();\n    return arr->at(0);",
    },
    {
      note: "the next frame uses another key",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  o.set(u"id", id);',
      to: '  o.set(u"iid", id);',
    },
    {
      note: "the next frame carries no id",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  o.set(u"id", id);\n',
      to: "",
    },
    {
      note: "landing ignores the frame on_landing",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }\n",
      to: "",
    },
    {
      note: "landing prefers the index over the frame on_landing",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
      to: "  if (!truthy(landing)) {\n    e.enter_frame(landing);\n    return;\n  }",
    },
    {
      note: "landing falls back to the default frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_2()));",
      to: "  e.enter_frame_by_id(to_string(e.data_indexes_default()));",
    },
    {
      note: "landing never enters a frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.enter_frame_by_id(to_string(e.data_indexes_landing_2()));\n",
      to: "",
    },
    {
      note: "a heavy holding does not win the auto frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  Value fid;\n  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "  Value fid;\n  if (strict_equals(e.holding_base_type(), Value(1.0))) {",
    },
    {
      note: "the heavy walk frame is read from another index",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    fid = e.data_indexes_heavy_obj_walk();",
      to: "    fid = e.data_indexes_default();",
    },
    {
      note: "the holding test is loose",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  Value fid;\n  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "  Value fid;\n  if (equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "being on the ground is not checked",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  } else if (e.is_on_ground()) {",
      to: "  } else if (!e.is_on_ground()) {",
    },
    {
      note: "a dying entity still uses the sky frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  } else if (to_number(e.hp()) > 0) {",
      to: "  } else if (to_number(e.hp()) >= 0) {",
    },
    {
      note: "a dying airborne entity yields nothing",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  } else if (to_number(e.hp()) > 0) {\n    fid = index_0(e.data_indexes_in_the_skys());\n  }",
      to: "  }",
    },
    {
      note: "the sky frame is not indexed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    fid = index_0(e.data_indexes_in_the_skys());",
      to: "    fid = e.data_indexes_default();",
    },
    {
      note: "the frame is looked up with the default id",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  return index_by(e.data_frames(), to_string(fid));",
      to: "  return index_by(e.data_frames(), to_string(e.data_indexes_default()));",
    },
    {
      note: "the frame map is replaced by an index",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  return index_by(e.data_frames(), to_string(fid));",
      to: "  return index_by(e.data_indexes_default(), to_string(fid));",
    },
    {
      note: "a false frame id still yields a frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (!truthy(fid)) return Value();",
      to: "  if (truthy(fid)) return Value();",
    },
    {
      note: "the sudden death velocity uses another factor",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.set_velocity(Value(2 * to_number(e.facing())), Value(2.0), Value());",
      to: "  e.set_velocity(Value(3 * to_number(e.facing())), Value(2.0), Value());",
    },
    {
      note: "the sudden death y velocity is another value",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.set_velocity(Value(2 * to_number(e.facing())), Value(2.0), Value());",
      to: "  e.set_velocity(Value(2 * to_number(e.facing())), Value(3.0), Value());",
    },
    {
      note: "the sudden death frame is read from the up facing",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));',
      to: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"1"));',
    },
    {
      note: "the sudden death frame takes the first entry",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));',
      to: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"0"));',
    },
    {
      note: "the sudden death frame ignores the falling list",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  const Value falling = e.data_indexes_falling();\n  if (truthy(falling)) return next_frame(index_by(index_by(falling, u\"1\"), u\"1\"));",
      to: "  const Value falling = e.data_indexes_falling();\n  return next_frame(index_by(index_by(falling, u\"1\"), u\"1\"));",
    },
    {
      note: "a missing falling list still yields a frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));',
      to: '  if (!truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));',
    },
    {
      note: "the sudden death frame is never produced",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));\n',
      to: "",
    },
    {
      note: "the caught end x velocity uses another factor",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.set_velocity(Value(-1 * to_number(cvx) * to_number(e.facing())), cvy, Value());",
      to: "  e.set_velocity(Value(1 * to_number(cvx) * to_number(e.facing())), cvy, Value());",
    },
    {
      note: "the caught end y velocity takes the x dataset",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  const Value cvy = e.dataset(u\"cvy_d\");",
      to: "  const Value cvy = e.dataset(u\"cvx_d\");",
    },
    {
      note: "the caught end x velocity takes the y dataset",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  const Value cvx = e.dataset(u\"cvx_d\");",
      to: "  const Value cvx = e.dataset(u\"cvy_d\");",
    },
    {
      note: "the caught end velocity arguments are swapped",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.set_velocity(Value(-1 * to_number(cvx) * to_number(e.facing())), cvy, Value());",
      to: "  e.set_velocity(Value(-1 * to_number(cvx) * to_number(e.facing())), Value(), cvy);",
    },
    {
      note: "the caught end frame is read from the down facing",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"1"));',
      to: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));',
    },
    {
      note: "the caught end frame takes the first entry",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"1"));',
      to: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"0"));',
    },
    {
      note: "the caught end frame ignores the falling list",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  const Value falling = e.data_indexes_falling();\n  if (truthy(falling)) return next_frame(index_by(index_by(falling, u\"-1\"), u\"1\"));",
      to: "  const Value falling = e.data_indexes_falling();\n  return next_frame(index_by(index_by(falling, u\"-1\"), u\"1\"));",
    },
    {
      note: "the caught end frame is never produced",
      file: "native/lfw/state/character_state_base.cpp",
      from: '  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"1"));\n',
      to: "",
    },
    {
      note: "leaving the ground accepts any state",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Running))) &&",
      to: "  if (!equals(st, Value(static_cast<double>(StateEnum::Running))) &&",
    },
    {
      note: "leaving the ground ignores the running state",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Running))) &&\n",
      to: "  if (",
    },
    {
      note: "leaving the ground ignores the walking state",
      file: "native/lfw/state/character_state_base.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Walking))) &&\n",
      to: "",
    },
    {
      note: "leaving the ground ignores the standing state",
      file: "native/lfw/state/character_state_base.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Standing))) &&\n",
      to: "",
    },
    {
      note: "leaving the ground ignores the rowing state",
      file: "native/lfw/state/character_state_base.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Rowing)))) {",
      to: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Falling)))) {",
    },
    {
      note: "the four states are combined with or",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Running))) &&\n      !strict_equals(st, Value(static_cast<double>(StateEnum::Walking))) &&",
      to: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Running))) ||\n      !strict_equals(st, Value(static_cast<double>(StateEnum::Walking))) &&",
    },
    {
      note: "a heavy holding is not dropped",
      file: "native/lfw/state/character_state_base.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Rowing)))) {\n    return;\n  }\n  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n    e.drop_holding();\n  }",
      to: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Rowing)))) {\n    return;\n  }\n  if (strict_equals(e.holding_base_type(), Value(1.0))) {\n    e.drop_holding();\n  }",
    },
    {
      note: "the holding is never dropped",
      file: "native/lfw/state/character_state_base.cpp",
      from: "    e.drop_holding();\n",
      to: "",
    },
    {
      note: "leaving the ground enters another frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));",
      to: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kSelf))));",
    },
    {
      note: "leaving the ground enters no frame",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));\n",
      to: "",
    },
    {
      note: "the ground velocity decay is not driven",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  e.handle_ground_velocity_decay();\n}",
      to: "}",
    },
    {
      note: "the on_landing hook is not installed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  on_landing = &csb_on_landing;",
      to: "  on_landing = nullptr;",
    },
    {
      note: "the auto frame hook is not installed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  get_auto_frame = &csb_get_auto_frame;",
      to: "  get_auto_frame = nullptr;",
    },
    {
      note: "the sudden death hook is not installed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  get_sudden_death_frame = &csb_get_sudden_death_frame;",
      to: "  get_sudden_death_frame = nullptr;",
    },
    {
      note: "the caught end hook is not installed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  get_caught_end_frame = &csb_get_caught_end_frame;",
      to: "  get_caught_end_frame = nullptr;",
    },
    {
      note: "the leave ground hook is not installed",
      file: "native/lfw/state/character_state_base.cpp",
      from: "  on_leave_ground = &csb_on_leave_ground;",
      to: "  on_leave_ground = nullptr;",
    },
  ],
};
