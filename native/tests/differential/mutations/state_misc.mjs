/**
 * Mutation spec for `native/lfw/state/state_misc.{h,cpp}` — the five small states
 * `State_WeaponBroken` / `State_TransformToCatching` / `CharacterState_TransformToLouisEX`
 * / `State_TransformTo8XXX` / `BallState_Base`.
 *
 * Disproven / unobservable mutations (proven by reading the TS originals, the port
 * and the case file, not by a surviving run):
 *
 * 1. The default arguments of `State_WeaponBroken` / `State_TransformToCatching` /
 *    `CharacterState_TransformToLouisEX` are unobservable: none of them reads the
 *    state object's own `_state`. (`State_TransformTo8XXX` *does* read it, and that
 *    read is covered by the `typeof` guard mutations.)
 * 2. `CharacterState_TransformToLouisEX`'s TS class name and its file name disagree
 *    (`CharacterState_TransformToLouisEX` in `CharacterState_Transform2LouisEX.ts`);
 *    the port follows the class name.
 * 3. `BallState_Base` has no constructor in the TS original (it inherits
 *    `State_Base`'s); the port adds one only to install the `enter` hook.
 * 4. `e.world.callbacks.call("on_fighter_add", e)` passes `e` as a second argument,
 *    which the port's seam drops; the callback payload is not observable here.
 * 5. `gone_frame_info()` renders the whole `GONE_FRAME_INFO` object literal; the
 *    harness only logs it, so a mutation *inside* that constant belongs to the
 *    `defines` unit, not this one.
 */
export default {
  subject: "state_misc",
  mutations: [
    {
      note: "weapon broken enters an empty frame",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.enter_frame(gone_frame_info());",
      to: "  e.enter_frame(Value());",
    },
    {
      note: "weapon broken enters no frame on landing",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.enter_frame(gone_frame_info());\n",
      to: "",
    },
    {
      note: "the weapon broken landing hook is not installed",
      file: "native/lfw/state/state_misc.cpp",
      from: "  on_landing = &swb_on_landing;",
      to: "  on_landing = nullptr;",
    },
    {
      note: "the Louis EX fighter id is another one",
      file: "native/lfw/state/state_misc.cpp",
      from: '  const Value d = e.datas_find_fighter(u"50");',
      to: '  const Value d = e.datas_find_fighter(u"51");',
    },
    {
      note: "a missing fighter data still transforms",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!truthy(d)) return;\n",
      to: "",
    },
    {
      note: "a found fighter data aborts the transform",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!truthy(d)) return;",
      to: "  if (truthy(d)) return;",
    },
    {
      note: "the fighter data is not applied",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transform(d);\n",
      to: "",
    },
    {
      note: "the transform receives nothing",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transform(d);",
      to: "  e.transform(Value());",
    },
    {
      note: "the auto frame is not entered after the transform",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transform(d);\n  e.enter_frame(e.find_auto_frame());\n",
      to: "  e.transform(d);\n",
    },
    {
      note: "the transform enters an empty frame",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transform(d);\n  e.enter_frame(e.find_auto_frame());",
      to: "  e.transform(d);\n  e.enter_frame(Value());",
    },
    {
      note: "the Louis EX enter hook is not installed",
      file: "native/lfw/state/state_misc.cpp",
      from: "  enter = &cs2_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the first ball state is not tested",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&\n",
      to: "  if (",
    },
    {
      note: "the second ball state is not tested",
      file: "native/lfw/state/state_misc.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hit))) &&\n",
      to: "",
    },
    {
      note: "the third ball state is not tested",
      file: "native/lfw/state/state_misc.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Rebounding))) &&\n",
      to: "",
    },
    {
      note: "the fourth ball state is not tested",
      file: "native/lfw/state/state_misc.cpp",
      from: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Disappear)))) {",
      to: "      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_3005)))) {",
    },
    {
      note: "the first ball state test is loose",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&",
      to: "  if (!equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&",
    },
    {
      note: "the first ball state is another one",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&",
      to: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Flying))) &&",
    },
    {
      note: "the four ball states are combined with or",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&\n      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hit))) &&",
      to: "  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) ||\n      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hit))) &&",
    },
    {
      note: "the shaking is set to one",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_shaking(Value(0.0));",
      to: "  e.set_shaking(Value(1.0));",
    },
    {
      note: "the shaking is never cleared",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_shaking(Value(0.0));\n",
      to: "",
    },
    {
      note: "the motionless flag is set to one",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_motionless(Value(0.0));",
      to: "  e.set_motionless(Value(1.0));",
    },
    {
      note: "the motionless flag is never cleared",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_motionless(Value(0.0));\n",
      to: "",
    },
    {
      note: "the ball velocity is not zeroed",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));\n",
      to: "",
    },
    {
      note: "the ball velocity keeps its x",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));",
      to: "  e.set_velocity(Value(1.0), Value(0.0), Value(0.0));",
    },
    {
      note: "the ball enter hook is not installed",
      file: "native/lfw/state/state_misc.cpp",
      from: "  enter = &bsb_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the transform to catching does not transform",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transfrom_to_another();\n",
      to: "",
    },
    {
      note: "the transform to catching enters no frame",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transfrom_to_another();\n  e.enter_frame(e.find_auto_frame());",
      to: "  e.transfrom_to_another();\n  e.enter_frame(Value());",
    },
    {
      note: "the transform to catching never enters a frame",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.transfrom_to_another();\n  e.enter_frame(e.find_auto_frame());\n",
      to: "  e.transfrom_to_another();\n",
    },
    {
      note: "a non numeric state still leaves",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!std::holds_alternative<double>(state())) return;\n",
      to: "",
    },
    {
      note: "a numeric state aborts the leave",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!std::holds_alternative<double>(state())) return;",
      to: "  if (std::holds_alternative<double>(state())) return;",
    },
    {
      note: "the data id offsets the state up",
      file: "native/lfw/state/state_misc.cpp",
      from: "  const std::u16string oid = to_string(Value(to_number(state()) - 8000));",
      to: "  const std::u16string oid = to_string(Value(to_number(state()) + 8000));",
    },
    {
      note: "the data id offsets the state by one more",
      file: "native/lfw/state/state_misc.cpp",
      from: "  const std::u16string oid = to_string(Value(to_number(state()) - 8000));",
      to: "  const std::u16string oid = to_string(Value(to_number(state()) - 8001));",
    },
    {
      note: "the data lookup is ignored",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (truthy(data)) e.transform(data);\n",
      to: "",
    },
    {
      note: "the transform receives nothing",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (truthy(data)) e.transform(data);",
      to: "  if (truthy(data)) e.transform(Value());",
    },
    {
      note: "the old data type is not remembered",
      file: "native/lfw/state/state_misc.cpp",
      from: "  const Value old_data = e.data();",
      to: "  const Value old_data = Value();",
    },
    {
      note: "the auto frame is not entered after leaving",
      file: "native/lfw/state/state_misc.cpp",
      from: "  e.enter_frame(e.find_auto_frame());\n  const Value new_type = e.data_type();",
      to: "  const Value new_type = e.data_type();",
    },
    {
      note: "the type change test is inverted",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(lfw::field_or(old_data, u\"type\"), new_type) &&",
      to: "  if (strict_equals(lfw::field_or(old_data, u\"type\"), new_type) &&",
    },
    {
      note: "an unchanged type is still reported",
      file: "native/lfw/state/state_misc.cpp",
      from: "  if (!strict_equals(lfw::field_or(old_data, u\"type\"), new_type) &&\n",
      to: "  if (",
    },
    {
      note: "another new type is reported",
      file: "native/lfw/state/state_misc.cpp",
      from: "      strict_equals(new_type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
      to: "      strict_equals(new_type, Value(static_cast<double>(EntityEnum::Weapon)))) {",
    },
    {
      note: "the new fighter type test is loose",
      file: "native/lfw/state/state_misc.cpp",
      from: "      strict_equals(new_type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
      to: "      equals(new_type, Value(static_cast<double>(EntityEnum::Fighter)))) {",
    },
    {
      note: "another callback is raised",
      file: "native/lfw/state/state_misc.cpp",
      from: '    e.world_callbacks_call(u"on_fighter_add");',
      to: '    e.world_callbacks_call(u"on_fighter_add2");',
    },
    {
      note: "no callback is raised",
      file: "native/lfw/state/state_misc.cpp",
      from: '    e.world_callbacks_call(u"on_fighter_add");\n',
      to: "",
    },
  ],
};
