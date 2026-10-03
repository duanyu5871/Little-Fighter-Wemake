/**
 * Mutation spec for `native/lfw/collision/ball_frozen.cpp`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. The `std::u16string` arm of `group_length_truthy` (and the `false` fallback
 *    for it) cannot be driven from the case file: a non-empty *string* `group`
 *    satisfies `?.length` in TS and then dies in `group.some(...)` with a
 *    `TypeError`, so there is no TS output line to compare against. Only the
 *    `Array` arm is reachable, which is exactly why the "non array falls through
 *    as truthy" mutation below targets the final `return false;`.
 * 2. `a_facing > 0` -> `a_facing >= 0` (the `cx1` arm selector) is unobservable.
 *    The only value where `>` and `>=` differ is `0`, and that same `0` is the
 *    multiplier of `a_facing * round(cx2 - cx1)`, so the result is `+0` either way
 *    and `render` prints both as `0`. The `v_facing_number > 0` sibling *is*
 *    observable (the multiplier there is `a_facing`) and is included.
 * 3. Dropping the `o.set(u"z", Value(0.0));` initialiser is unobservable: the very
 *    next statement always overwrites `z` with a real number before `spawn` reads
 *    it, and the key order is never printed (the harness logs the six fields
 *    individually).
 * 4. Reordering the `is_ball` / `is_fighter` arms of the do/while is unobservable:
 *    the three arms are pure predicates combined by short-circuit `||`, the seam
 *    predicates have no side effects, and the only observable is *whether* the
 *    block falls through to `return false;`.
 * 5. `Value(std::u16string(...))` argument conversions, `field_or` key order and
 *    the `round` helper itself are shared infrastructure already covered by the
 *    `value` / `math` / `entity_helpers` subjects.
 */
export default {
  subject: "collision_ball_frozen",
  mutations: [
    {
      note: "group_length_truthy: array emptiness inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (const Array* arr = as_array(group)) return arr->size() != 0;",
      to: "  if (const Array* arr = as_array(group)) return arr->size() == 0;",
    },
    {
      note: "group_length_truthy: non array group is treated as non empty",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (const std::u16string* s = std::get_if<std::u16string>(&group)) return s->size() != 0;\n  return false;",
      to: "  if (const std::u16string* s = std::get_if<std::u16string>(&group)) return s->size() != 0;\n  return true;",
    },
    {
      note: "group_some_equal: null array treated as a match",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "bool group_some_equal(const Value& group, const char16_t* name) {\n  const Array* arr = as_array(group);\n  if (arr == nullptr) return false;",
      to: "bool group_some_equal(const Value& group, const char16_t* name) {\n  const Array* arr = as_array(group);\n  if (arr != nullptr) return false;",
    },
    {
      note: "group_some_equal: predicate inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  for (size_t i = 0; i < arr->size(); ++i) {\n    if (equals(arr->at(i), want)) return true;\n  }",
      to: "  for (size_t i = 0; i < arr->size(); ++i) {\n    if (!equals(arr->at(i), want)) return true;\n  }",
    },
    {
      note: "group_some_equal: first element ignored",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  for (size_t i = 0; i < arr->size(); ++i) {\n    if (equals(arr->at(i), want)) return true;\n  }",
      to: "  for (size_t i = 1; i < arr->size(); ++i) {\n    if (equals(arr->at(i), want)) return true;\n  }",
    },
    {
      note: "group_some_equal: last element ignored",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  for (size_t i = 0; i < arr->size(); ++i) {\n    if (equals(arr->at(i), want)) return true;\n  }",
      to: "  for (size_t i = 0; i + 1 < arr->size(); ++i) {\n    if (equals(arr->at(i), want)) return true;\n  }",
    },
    {
      note: "group_some_equal: falls back to true",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    if (equals(arr->at(i), want)) return true;\n  }\n  return false;",
      to: "    if (equals(arr->at(i), want)) return true;\n  }\n  return true;",
    },
    {
      note: "group_some_not_equal: null array treated as a match",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "bool group_some_not_equal(const Value& group, const char16_t* name) {\n  const Array* arr = as_array(group);\n  if (arr == nullptr) return false;",
      to: "bool group_some_not_equal(const Value& group, const char16_t* name) {\n  const Array* arr = as_array(group);\n  if (arr != nullptr) return false;",
    },
    {
      note: "group_some_not_equal: predicate inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    if (!equals(arr->at(i), want)) return true;",
      to: "    if (equals(arr->at(i), want)) return true;",
    },
    {
      note: "group_some_not_equal: first element ignored",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  for (size_t i = 0; i < arr->size(); ++i) {\n    if (!equals(arr->at(i), want)) return true;\n  }",
      to: "  for (size_t i = 1; i < arr->size(); ++i) {\n    if (!equals(arr->at(i), want)) return true;\n  }",
    },
    {
      note: "group_some_not_equal: falls back to true",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    if (!equals(arr->at(i), want)) return true;\n  }\n  return false;",
      to: "    if (!equals(arr->at(i), want)) return true;\n  }\n  return true;",
    },
    {
      note: "opoint: oid key misspelled",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '    o.set(u"oid", Value(std::u16string(oid::kFreezeBall)));',
      to: '    o.set(u"oids", Value(std::u16string(oid::kFreezeBall)));',
    },
    {
      note: "opoint: oid value replaced",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '    o.set(u"oid", Value(std::u16string(oid::kFreezeBall)));',
      to: '    o.set(u"oid", Value(std::u16string(u"210")));',
    },
    {
      note: "opoint: kind value replaced",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '    o.set(u"kind", Value(0.0));',
      to: '    o.set(u"kind", Value(1.0));',
    },
    {
      note: "opoint: action replaced",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '    action.set(u"id", Value(std::u16string(frame_id::kAuto)));',
      to: '    action.set(u"id", Value(std::u16string(frame_id::kSelf)));',
    },
    {
      note: "itr: kind key misspelled",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const Value itr_kind = field_or(itr, u"kind");',
      to: '  const Value itr_kind = field_or(itr, u"kinds");',
    },
    {
      note: "kind filter: Normal replaced by Catch",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) ||",
      to: "  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Catch))) ||",
    },
    {
      note: "kind filter: CharacterThrew arm dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew))) ||\n                  equals(itr_kind, Value(static_cast<double>(ItrKind::WeaponSwing)));",
      to: "                  equals(itr_kind, Value(static_cast<double>(ItrKind::WeaponSwing)));",
    },
    {
      note: "kind filter: WeaponSwing arm dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew))) ||\n                  equals(itr_kind, Value(static_cast<double>(ItrKind::WeaponSwing)));",
      to: "                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew)));",
    },
    {
      note: "kind filter: first two arms combined with and",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) ||\n                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew))) ||",
      to: "  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) &&\n                  equals(itr_kind, Value(static_cast<double>(ItrKind::CharacterThrew))) ||",
    },
    {
      note: "kind filter: loose comparison made strict",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool ok = equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) ||",
      to: "  const bool ok = strict_equals(itr_kind, Value(static_cast<double>(ItrKind::Normal))) ||",
    },
    {
      note: "swap: freezer test uses the freezable name",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (group_some_equal(v_group, entity_group::kFreezer) &&",
      to: "  if (group_some_equal(v_group, entity_group::kFreezableBall) &&",
    },
    {
      note: "swap: freezable test uses the freezer name",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "      group_some_equal(a_group, entity_group::kFreezableBall)) {",
      to: "      group_some_equal(a_group, entity_group::kFreezer)) {",
    },
    {
      note: "swap: victim and attacker groups swapped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (group_some_equal(v_group, entity_group::kFreezer) &&\n      group_some_equal(a_group, entity_group::kFreezableBall)) {",
      to: "  if (group_some_equal(a_group, entity_group::kFreezer) &&\n      group_some_equal(v_group, entity_group::kFreezableBall)) {",
    },
    {
      note: "swap: condition uses or",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (group_some_equal(v_group, entity_group::kFreezer) &&\n      group_some_equal(a_group, entity_group::kFreezableBall)) {",
      to: "  if (group_some_equal(v_group, entity_group::kFreezer) ||\n      group_some_equal(a_group, entity_group::kFreezableBall)) {",
    },
    {
      note: "swap: swap is skipped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    IFrozenEntity* temp = a;\n    a = v;\n    v = temp;\n  } else if (",
      to: "  } else if (",
    },
    {
      note: "swap: swap keeps the attacker for both roles",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    IFrozenEntity* temp = a;\n    a = v;\n    v = temp;",
      to: "    IFrozenEntity* temp = a;\n    a = v;\n    v = temp;\n    v = a;",
    },
    {
      note: "group gate: else if becomes a second if",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  } else if (group_some_not_equal(a_group, entity_group::kFreezer) ||",
      to: "  }\n  if (group_some_not_equal(a_group, entity_group::kFreezer) ||",
    },
    {
      note: "group gate: not-freezer test uses the freezable name",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  } else if (group_some_not_equal(a_group, entity_group::kFreezer) ||",
      to: "  } else if (group_some_not_equal(a_group, entity_group::kFreezableBall) ||",
    },
    {
      note: "group gate: not-freezable test uses the freezer name",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "             group_some_not_equal(v_group, entity_group::kFreezableBall)) {",
      to: "             group_some_not_equal(v_group, entity_group::kFreezer)) {",
    },
    {
      note: "group gate: victim and attacker tests swapped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  } else if (group_some_not_equal(a_group, entity_group::kFreezer) ||\n             group_some_not_equal(v_group, entity_group::kFreezableBall)) {",
      to: "  } else if (group_some_not_equal(v_group, entity_group::kFreezer) ||\n             group_some_not_equal(a_group, entity_group::kFreezableBall)) {",
    },
    {
      note: "group gate: the two not-equal tests are combined with and",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  } else if (group_some_not_equal(a_group, entity_group::kFreezer) ||\n             group_some_not_equal(v_group, entity_group::kFreezableBall)) {",
      to: "  } else if (group_some_not_equal(a_group, entity_group::kFreezer) &&\n             group_some_not_equal(v_group, entity_group::kFreezableBall)) {",
    },
    {
      note: "victim state: flying compared against weapon on hand",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!equals(v->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;",
      to: "  if (!equals(v->state(), Value(static_cast<double>(StateEnum::Weapon_OnHand)))) return false;",
    },
    {
      note: "victim state: flying guard inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!equals(v->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;",
      to: "  if (equals(v->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;",
    },
    {
      note: "victim state: flying guard reads the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!equals(v->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;",
      to: "  if (!equals(a->state(), Value(static_cast<double>(StateEnum::Ball_Flying)))) return false;",
    },
    {
      note: "kind gate: ball arm inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!g_env.is_ball(*v)) {",
      to: "  if (g_env.is_ball(*v)) {",
    },
    {
      note: "kind gate: ball arm tests the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!g_env.is_ball(*v)) {",
      to: "  if (!g_env.is_ball(*a)) {",
    },
    {
      note: "kind gate: fighter arm inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    if (!g_env.is_fighter(*a)) {",
      to: "    if (g_env.is_fighter(*a)) {",
    },
    {
      note: "kind gate: fighter arm tests the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "    if (!g_env.is_fighter(*a)) {",
      to: "    if (!g_env.is_fighter(*v)) {",
    },
    {
      note: "kind gate: weapon on hand compared against weapon on ground",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "      if (!equals(a->state(), Value(static_cast<double>(StateEnum::Weapon_OnHand)))) return false;",
      to: "      if (!equals(a->state(), Value(static_cast<double>(StateEnum::Weapon_OnGround)))) return false;",
    },
    {
      note: "kind gate: weapon on hand guard inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "      if (!equals(a->state(), Value(static_cast<double>(StateEnum::Weapon_OnHand)))) return false;",
      to: "      if (equals(a->state(), Value(static_cast<double>(StateEnum::Weapon_OnHand)))) return false;",
    },
    {
      note: "attacker x comes from the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double x1 = a->position_x();",
      to: "  const double x1 = v->position_x();",
    },
    {
      note: "attacker y comes from the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double y1 = a->position_y();",
      to: "  const double y1 = v->position_y();",
    },
    {
      note: "attacker z comes from the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double z1 = a->position_z();",
      to: "  const double z1 = v->position_z();",
    },
    {
      note: "attacker y is read as z",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double y1 = a->position_y();",
      to: "  const double y1 = a->position_z();",
    },
    {
      note: "attacker z is read as y",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double z1 = a->position_z();",
      to: "  const double z1 = a->position_y();",
    },
    {
      note: "victim x comes from the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double x2 = v->position_x();",
      to: "  const double x2 = a->position_x();",
    },
    {
      note: "victim y and z are read swapped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double y2 = v->position_y();\n  const double z2 = v->position_z();",
      to: "  const double y2 = v->position_z();\n  const double z2 = v->position_y();",
    },
    {
      note: "attacker frame comes from the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const Value a_frame = a->frame();",
      to: "  const Value a_frame = v->frame();",
    },
    {
      note: "victim frame comes from the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const Value v_frame = v->frame();",
      to: "  const Value v_frame = a->frame();",
    },
    {
      note: "attacker centerx is read as centery",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double a_centerx = frame_number(a_frame, u"centerx");',
      to: '  const double a_centerx = frame_number(a_frame, u"centery");',
    },
    {
      note: "attacker centery is read as centerx",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double a_centery = frame_number(a_frame, u"centery");',
      to: '  const double a_centery = frame_number(a_frame, u"centerx");',
    },
    {
      note: "victim centerx is read as centery",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double v_centerx = frame_number(v_frame, u"centerx");',
      to: '  const double v_centerx = frame_number(v_frame, u"centery");',
    },
    {
      note: "victim centery is read as centerx",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double v_centery = frame_number(v_frame, u"centery");',
      to: '  const double v_centery = frame_number(v_frame, u"centerx");',
    },
    {
      note: "victim width is read as height",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double v_width = frame_number(v_frame, u"width");',
      to: '  const double v_width = frame_number(v_frame, u"height");',
    },
    {
      note: "victim height is read as width",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  const double v_height = frame_number(v_frame, u"height");',
      to: '  const double v_height = frame_number(v_frame, u"width");',
    },
    {
      note: "attacker facing comes from the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double a_facing = to_number(a->facing());",
      to: "  const double a_facing = to_number(v->facing());",
    },
    {
      note: "victim facing comes from the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double v_facing_number = to_number(v->facing());",
      to: "  const double v_facing_number = to_number(a->facing());",
    },
    {
      note: "cy1 subtracts the vertical offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cy1 = y1 + a_centery;",
      to: "  const double cy1 = y1 - a_centery;",
    },
    {
      note: "cy1 uses the horizontal offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cy1 = y1 + a_centery;",
      to: "  const double cy1 = y1 + a_centerx;",
    },
    {
      note: "cx1 right-facing arm adds the offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cx1 = a_facing > 0 ? x1 - a_centerx : x1 + a_centerx - a_centerx;",
      to: "  const double cx1 = a_facing > 0 ? x1 + a_centerx : x1 + a_centerx - a_centerx;",
    },
    {
      note: "cx1 left-facing arm doubles the offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cx1 = a_facing > 0 ? x1 - a_centerx : x1 + a_centerx - a_centerx;",
      to: "  const double cx1 = a_facing > 0 ? x1 - a_centerx : x1 + a_centerx + a_centerx;",
    },
    {
      note: "cy2 keeps the victim height whole",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cy2 = (y2 + v_centery) - v_height / 2.0;",
      to: "  const double cy2 = (y2 + v_centery) - v_height * 2.0;",
    },
    {
      note: "cy2 adds the half height",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cy2 = (y2 + v_centery) - v_height / 2.0;",
      to: "  const double cy2 = (y2 + v_centery) + v_height / 2.0;",
    },
    {
      note: "cy2 subtracts the vertical offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cy2 = (y2 + v_centery) - v_height / 2.0;",
      to: "  const double cy2 = (y2 - v_centery) - v_height / 2.0;",
    },
    {
      note: "cx2 right-facing arm adds the offset",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cx2 = v_facing_number > 0 ? (x2 - v_centerx) + v_width / 2.0",
      to: "  const double cx2 = v_facing_number > 0 ? (x2 + v_centerx) + v_width / 2.0",
    },
    {
      note: "cx2 right-facing arm doubles the width",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cx2 = v_facing_number > 0 ? (x2 - v_centerx) + v_width / 2.0",
      to: "  const double cx2 = v_facing_number > 0 ? (x2 - v_centerx) + v_width * 2.0",
    },
    {
      note: "cx2 facing test includes zero",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const double cx2 = v_facing_number > 0 ? (x2 - v_centerx) + v_width / 2.0",
      to: "  const double cx2 = v_facing_number >= 0 ? (x2 - v_centerx) + v_width / 2.0",
    },
    {
      note: "cx2 left-facing arm adds the width",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "                                         : (x2 + v_centerx - v_width) + v_width / 2.0;",
      to: "                                         : (x2 + v_centerx + v_width) + v_width / 2.0;",
    },
    {
      note: "cx2 left-facing arm doubles the width",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "                                         : (x2 + v_centerx - v_width) + v_width / 2.0;",
      to: "                                         : (x2 + v_centerx - v_width) + v_width * 2.0;",
    },
    {
      note: "opoint x: facing multiplier dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"x", Value(a_facing * round(cx2 - cx1)));',
      to: '  opoint->set(u"x", Value(round(cx2 - cx1)));',
    },
    {
      note: "opoint x: delta negated",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"x", Value(a_facing * round(cx2 - cx1)));',
      to: '  opoint->set(u"x", Value(a_facing * round(cx1 - cx2)));',
    },
    {
      note: "opoint x: rounding dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"x", Value(a_facing * round(cx2 - cx1)));',
      to: '  opoint->set(u"x", Value(a_facing * (cx2 - cx1)));',
    },
    {
      note: "opoint x: written into y",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"x", Value(a_facing * round(cx2 - cx1)));',
      to: '  opoint->set(u"y", Value(a_facing * round(cx2 - cx1)));',
    },
    {
      note: "opoint y: delta negated",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"y", Value(round(cy1 - cy2)));',
      to: '  opoint->set(u"y", Value(round(cy2 - cy1)));',
    },
    {
      note: "opoint y: rounding dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"y", Value(round(cy1 - cy2)));',
      to: '  opoint->set(u"y", Value(cy1 - cy2));',
    },
    {
      note: "opoint y: written into z",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"y", Value(round(cy1 - cy2)));',
      to: '  opoint->set(u"z", Value(round(cy1 - cy2)));',
    },
    {
      note: "opoint z: delta negated",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"z", Value(round(z2 - z1)));',
      to: '  opoint->set(u"z", Value(round(z1 - z2)));',
    },
    {
      note: "opoint z: rounding dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"z", Value(round(z2 - z1)));',
      to: '  opoint->set(u"z", Value(z2 - z1));',
    },
    {
      note: "opoint z: written into x",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: '  opoint->set(u"z", Value(round(z2 - z1)));',
      to: '  opoint->set(u"x", Value(round(z2 - z1)));',
    },
    {
      note: "spawn: asked of the victim",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool freeze_ball = a->spawn(opoint_v, entity::turn_face(v->facing()));",
      to: "  const bool freeze_ball = v->spawn(opoint_v, entity::turn_face(v->facing()));",
    },
    {
      note: "spawn: face flip dropped",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool freeze_ball = a->spawn(opoint_v, entity::turn_face(v->facing()));",
      to: "  const bool freeze_ball = a->spawn(opoint_v, v->facing());",
    },
    {
      note: "spawn: face flip uses the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  const bool freeze_ball = a->spawn(opoint_v, entity::turn_face(v->facing()));",
      to: "  const bool freeze_ball = a->spawn(opoint_v, entity::turn_face(a->facing()));",
    },
    {
      note: "spawn: result guard inverted",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!freeze_ball) return false;",
      to: "  if (freeze_ball) return false;",
    },
    {
      note: "spawn: result ignored",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  if (!freeze_ball) return false;\n  v->enter_frame(gone_frame_info());",
      to: "  v->enter_frame(gone_frame_info());",
    },
    {
      note: "gone frame: not entered",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  v->enter_frame(gone_frame_info());\n  return true;",
      to: "  return true;",
    },
    {
      note: "gone frame: entered on the attacker",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  v->enter_frame(gone_frame_info());",
      to: "  a->enter_frame(gone_frame_info());",
    },
    {
      note: "gone frame: info replaced by undefined",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  v->enter_frame(gone_frame_info());",
      to: "  v->enter_frame(Value());",
    },
    {
      note: "success reported as failure",
      file: "native/lfw/collision/ball_frozen.cpp",
      from: "  v->enter_frame(gone_frame_info());\n  return true;",
      to: "  v->enter_frame(gone_frame_info());\n  return false;",
    },
  ],
};
