/**
 * Mutation spec for `native/lfw/state/character_state_walking.{h,cpp}`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. The constructor's default argument (`StateEnum.Walking`) is unobservable:
 *    nothing in this unit reads the state object's own `_state`.
 * 2. `const { UD, LR } = e.ctrl` throws when `ctrl` is missing; the port models it as
 *    two `bool` seams (`ctrl_ud()` / `ctrl_lr()`), so the "missing ctrl" case cannot
 *    be built and is not covered.
 * 3. `index_0`'s object branch is dead (`in_the_skys` is always an array).
 * 4. `enter_frame_by_id(undefined, true)` (a missing `indexes.default`) makes the real
 *    Entity substitute `FrameId.Auto`; the harness seam only logs the id, so the case
 *    always supplies `default`.
 * 5. `e.wait = e.handle_wait_flag(...)` is a plain property write on the entity; the
 *    harness logs it through the `set_wait` seam, which is the same observation.
 * 6. Swapping only the *x* and *z* outputs of `e.position(...)` is unobservable: the
 *    branch reads `py` only, and that slot is untouched by the swap.
 */
export default {
  subject: "character_state_walking",
  mutations: [
    {
      note: "walking does not decay the ground velocity",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "void CharacterState_Walking::update(IStateEntity& e) {\n  CharacterState_Base::update(e);",
      to: "void CharacterState_Walking::update(IStateEntity& e) {",
    },
    {
      note: "the up/down control is ignored",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_lr() && !truthy(e.wait())) {",
    },
    {
      note: "the left/right control is ignored",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_ud() && !truthy(e.wait())) {",
    },
    {
      note: "the wait is ignored",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_ud() && !e.ctrl_lr()) {",
    },
    {
      note: "the three idle conditions are combined with or",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_ud() || !e.ctrl_lr() && !truthy(e.wait())) {",
    },
    {
      note: "pressing up counts as idling",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
    },
    {
      note: "pressing left/right counts as idling",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_ud() && e.ctrl_lr() && !truthy(e.wait())) {",
    },
    {
      note: "a waiting entity still idles",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {",
      to: "  if (!e.ctrl_ud() && !e.ctrl_lr() && truthy(e.wait())) {",
    },
    {
      note: "the idle branch never runs",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (e.holding_is_weapon() &&\n        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n      e.set_wait(Value(e.handle_wait_flag(Value(), e.frame_info())));\n    } else {\n      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);\n    }\n",
      to: "",
    },
    {
      note: "a non weapon holding is accepted",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (e.holding_is_weapon() &&",
      to: "    if (!e.holding_is_weapon() &&",
    },
    {
      note: "the weapon test is skipped",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (e.holding_is_weapon() &&\n",
      to: "    if (",
    },
    {
      note: "the heavy holding test is loose",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "        equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "another weapon type is treated as heavy",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "        strict_equals(e.holding_base_type(), Value(1.0))) {",
    },
    {
      note: "the two idle arms are swapped",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (e.holding_is_weapon() &&\n        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "    if (!(e.holding_is_weapon() &&\n          strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy))))) {",
    },
    {
      note: "the wait flag is fed the frame first",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.set_wait(Value(e.handle_wait_flag(Value(), e.frame_info())));",
      to: "      e.set_wait(Value(e.handle_wait_flag(e.frame_info(), Value())));",
    },
    {
      note: "the wait flag is not consulted",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.set_wait(Value(e.handle_wait_flag(Value(), e.frame_info())));",
      to: "      e.set_wait(Value(0.0));",
    },
    {
      note: "the wait is not refreshed",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.set_wait(Value(e.handle_wait_flag(Value(), e.frame_info())));\n",
      to: "",
    },
    {
      note: "the default frame is entered without a fallback",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);",
      to: "      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), false);",
    },
    {
      note: "the default frame ignores the fallback seam",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);",
      to: "      e.enter_frame_by_id(to_string(e.data_indexes_default()));",
    },
    {
      note: "the default frame id is read from the hp",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);",
      to: "      e.enter_frame_by_id_fallback(to_string(e.hp()), true);",
    },
    {
      note: "the default frame is never entered",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);\n",
      to: "",
    },
    {
      note: "walking ignores a zero hp",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (to_number(e.hp()) <= 0) {",
      to: "  if (to_number(e.hp()) < 0) {",
    },
    {
      note: "walking treats a positive hp as dead",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (to_number(e.hp()) <= 0) {",
      to: "  if (to_number(e.hp()) > 0) {",
    },
    {
      note: "walking falls through the sudden death frame",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }",
      to: "    e.enter_frame(e.get_sudden_death_frame());\n  }",
    },
    {
      note: "walking enters an empty sudden death frame",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    e.enter_frame(e.get_sudden_death_frame());",
      to: "    e.enter_frame(Value());",
    },
    {
      note: "walking never enters the sudden death frame",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (to_number(e.hp()) <= 0) {\n    e.enter_frame(e.get_sudden_death_frame());\n    return;\n  }\n",
      to: "",
    },
    {
      note: "walking compares the wrong position axis",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (px > to_number(e.ground_y())) {",
    },
    {
      note: "walking accepts being exactly on the ground",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (py >= to_number(e.ground_y())) {",
    },
    {
      note: "walking compares the hp as the ground",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  if (py > to_number(e.ground_y())) {",
      to: "  if (py > to_number(e.hp())) {",
    },
    {
      note: "walking compares the z position as the y position",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "  e.position(px, py, pz);",
      to: "  e.position(px, pz, py);",
    },
    {
      note: "walking uses the default frame instead of the sky frame",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(e.data_indexes_in_the_skys())));",
      to: "    e.enter_frame_by_id(to_string(e.data_indexes_default()));",
    },
    {
      note: "walking never enters the sky frame",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    e.enter_frame_by_id(to_string(index_0(e.data_indexes_in_the_skys())));\n",
      to: "",
    },
    {
      note: "the sky frame takes the second entry",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() == 0) return Value();\n    return arr->at(1);",
    },
    {
      note: "index_0 rejects every non empty list",
      file: "native/lfw/state/character_state_walking.cpp",
      from: "    if (arr->size() == 0) return Value();\n    return arr->at(0);",
      to: "    if (arr->size() > 0) return Value();\n    return arr->at(0);",
    },
  ],
};
