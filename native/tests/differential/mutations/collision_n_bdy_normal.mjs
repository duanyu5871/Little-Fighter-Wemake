/**
 * Mutation spec for `native/lfw/collision/n_bdy_normal.cpp`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `if (a == nullptr || v == nullptr) return;` and the `v != nullptr &&` guard in
 *    the Ice arm -> dropped line, `&&`, or inverted to `!=`. The harness always
 *    registers both ids (`A` and `V`), so a lookup can never return null and the
 *    variants are indistinguishable. These guards are port artefacts: the TS
 *    original destructures `collision.attacker/victim` up front.
 * 2. The `else { g_env.spark(..., kSilentHit); }` arm of the impact spark is
 *    unreachable. It is only taken when `is_fighter(victim)` is false, but the
 *    impact path is only reached after `if (g_env.is_fall(c)) { ...; return; }`, and
 *    `is_fall` itself returns true as soon as `!is_fighter(victim)`. A non-fighter
 *    victim therefore always leaves through `handle_fall`.
 * 3. `if (truthy(id))` is redundant with the following
 *    `len.has_value() && *len > 0`: `value_length` returns `std::nullopt` for every
 *    non-string, non-array value (including `undefined`), so the outer truthiness
 *    test can never change the outcome.
 * 4. `else if (v_is_fighter)` -> `else`. Removing the test merges the `Hit` and
 *    `SilentHit` arms, but `SilentHit` is itself unreachable (see 2), so nothing
 *    observable changes.
 * 5. `truthy(...) && ...` prefixes inside the sub-handlers (`handle_armor`,
 *    `handle_fall`) belong to those units, not this one.
 * 6. `index_by`'s bounds guard `if (!(index >= 0) || index >= size) return Value();`.
 *    The only keys produced by `hurt_frame_group` are `"1"` and `"-1"`: for `"1"`
 *    both sides of a moved bound still admit the lookup, and for `"-1"` both sides
 *    still reject it. Widening it instead would make the `"-1"` path call
 *    `Array::at` with a negative index, i.e. undefined behaviour rather than a
 *    defined divergence, so it is excluded on purpose.
 * 7. `if (arr->size() == 0) return Value();` inside `index_0` -> changing the bound
 *    reads `Array::at(0)` out of bounds for the empty-list cases.
 * 8. `const INbdyNormalEntity* v = g_env.find_entity(c.vid);` inside the Ice arm is
 *    a port artefact (the TS original already has `victim` in hand); its guard is
 *    covered by 1.
 * 9. `equals(effect_v, ...Ignore)` -> `strict_equals`. Loose and strict equality
 *    only differ for a value that coerces to `10000` without being the number, e.g.
 *    the string `"10000"`. That case IS observable (armour then runs), so the
 *    mutation is included below rather than documented away.
 * 10. `value_length`'s string branch: `s->size()` -> `s->size() + 1`. The only way a
 *    string `id` can have length 0 is `""`, but `if (truthy(id))` in front of the
 *    length test rejects the empty string (JS `""` is falsy, and the port's
 *    `truthy` agrees), so the branch can only ever see strings of length >= 1. The
 *    array branch is different: `[]` is truthy, so an empty list DOES reach
 *    `value_length` and that off-by-one is observable and included below.
 * 11. `if (effect_is(effect_v, ItrEffect::Sharp) && v_is_fighter)` -> dropping
 *    `&& v_is_fighter`. This is the same unreachability as (2): the arm is only
 *    taken with a non-fighter victim, which always leaves through `handle_fall`
 *    first. The `else if (v_is_fighter)` arm keeps its own mutation.
 */
export default {
  subject: "collision_n_bdy_normal",
  mutations: [
    {
      note: "effect_is: compares against a shifted enum",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  return strict_equals(effect, Value(static_cast<double>(want)));",
      to: "  return strict_equals(effect, Value(static_cast<double>(want) + 1.0));",
    },
    {
      note: "hurt_frame_group: divides by 25 instead of 50",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double a = std::fmod(lfw::floor(r / 50.0), 2.0);",
      to: "  const double a = std::fmod(lfw::floor(r / 25.0), 2.0);",
    },
    {
      note: "hurt_frame_group: modulo three",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double a = std::fmod(lfw::floor(r / 50.0), 2.0);",
      to: "  const double a = std::fmod(lfw::floor(r / 50.0), 3.0);",
    },
    {
      note: "hurt_frame_group: modulo dropped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double a = std::fmod(lfw::floor(r / 50.0), 2.0);",
      to: "  const double a = lfw::floor(r / 50.0);",
    },
    {
      note: "hurt_frame_group: floor dropped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double a = std::fmod(lfw::floor(r / 50.0), 2.0);",
      to: "  const double a = std::fmod(r / 50.0, 2.0);",
    },
    {
      note: "hurt_frame_group: comparison is not strict",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  return a > 0 ? 1.0 : -1.0;",
      to: "  return a >= 0 ? 1.0 : -1.0;",
    },
    {
      note: "hurt_frame_group: branches swapped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  return a > 0 ? 1.0 : -1.0;",
      to: "  return a > 0 ? -1.0 : 1.0;",
    },
    {
      note: "hurt_frame_group: positive group is two",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  return a > 0 ? 1.0 : -1.0;",
      to: "  return a > 0 ? 2.0 : -1.0;",
    },
    {
      note: "hurt_frame_group: negative group is minus two",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  return a > 0 ? 1.0 : -1.0;",
      to: "  return a > 0 ? 1.0 : -2.0;",
    },
    {
      note: "value_length: string branch disabled",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const std::u16string* s = std::get_if<std::u16string>(&v);\n  if (s != nullptr) return static_cast<double>(s->size());",
      to: "  const std::u16string* s = nullptr;\n  if (s != nullptr) return static_cast<double>(s->size());",
    },
    {
      note: "value_length: array branch disabled",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const Array* arr = as_array(v);\n  if (arr != nullptr) return static_cast<double>(arr->size());",
      to: "  const Array* arr = nullptr;\n  if (arr != nullptr) return static_cast<double>(arr->size());",
    },
    {
      note: "value_length: array length is off by one",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (arr != nullptr) return static_cast<double>(arr->size());",
      to: "  if (arr != nullptr) return static_cast<double>(arr->size() + 1);",
    },
    {
      note: "index_by: object branch disabled",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const Object* o = as_object(holder);\n  if (o != nullptr) {\n    const Value* p = o->get(k);\n    if (p != nullptr) return *p;\n  }",
      to: "  const Object* o = nullptr;\n  if (o != nullptr) {\n    const Value* p = o->get(k);\n    if (p != nullptr) return *p;\n  }",
    },
    {
      note: "index_by: array element index forced to zero",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    return arr->at(static_cast<size_t>(index));",
      to: "    return arr->at(0);",
    },
    {
      note: "index_by: array branch disabled",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const Array* arr = as_array(holder);\n  if (arr != nullptr) {\n    const double index = to_number(Value(k));",
      to: "  const Array* arr = nullptr;\n  if (arr != nullptr) {\n    const double index = to_number(Value(k));",
    },
    {
      note: "first helper drops the injury handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_rest(c);",
    },
    {
      note: "first helper drops the rest handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_fall(c);\n}",
    },
    {
      note: "first helper drops the stiffness handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_fall(c);\n}",
    },
    {
      note: "first helper drops the fall handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n}",
    },
    {
      note: "first helper scales the injury by two",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 2.0, false);",
    },
    {
      note: "first helper keeps the toughness",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, false);",
      to: "void apply_injury_rest_stiffness_fall(Collision& c) {\n  handle_injury(c, 1.0, true);",
    },
    {
      note: "second helper drops the injury handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);",
      to: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_stiffness(c);",
    },
    {
      note: "second helper drops the stiffness handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_rest(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_fall(c);\n}",
    },
    {
      note: "second helper drops the rest handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_rest(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_fall(c);\n}",
    },
    {
      note: "second helper drops the fall handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_rest(c);\n  handle_fall(c);\n}",
      to: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n  handle_rest(c);\n}",
    },
    {
      note: "second helper scales the injury by two",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 1.0, false);",
      to: "void apply_injury_stiffness_rest_fall(Collision& c) {\n  handle_injury(c, 2.0, false);",
    },
    {
      note: "impact path drops the injury handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
      to: "  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
    },
    {
      note: "impact path drops the rest handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
      to: "  handle_injury(c, 1.0, false);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
    },
    {
      note: "impact path drops the stiffness handler",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
      to: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
    },
    {
      note: "impact path scales the injury by two",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
      to: "  handle_injury(c, 2.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
    },
    {
      note: "impact path keeps the toughness",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  handle_injury(c, 1.0, false);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
      to: "  handle_injury(c, 1.0, true);\n  handle_rest(c);\n  handle_stiffness(c);\n\n  INbdyNormalEntity* a = g_env.find_entity(c.aid);",
    },
    {
      note: "attacker resolved from the victim id",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  INbdyNormalEntity* a = g_env.find_entity(c.aid);\n  INbdyNormalEntity* v = g_env.find_entity(c.vid);",
      to: "  INbdyNormalEntity* a = g_env.find_entity(c.vid);\n  INbdyNormalEntity* v = g_env.find_entity(c.vid);",
    },
    {
      note: "victim resolved from the attacker id",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  INbdyNormalEntity* a = g_env.find_entity(c.aid);\n  INbdyNormalEntity* v = g_env.find_entity(c.vid);",
      to: "  INbdyNormalEntity* a = g_env.find_entity(c.aid);\n  INbdyNormalEntity* v = g_env.find_entity(c.aid);",
    },
    {
      note: "fall value decreases instead of accumulating",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_fall_value(Value(to_number(v->fall_value()) - to_number(a->itr_fall(c.itr))));",
      to: "  v->set_fall_value(Value(to_number(v->fall_value()) + to_number(a->itr_fall(c.itr))));",
    },
    {
      note: "fall value uses the victim itr_fall",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_fall_value(Value(to_number(v->fall_value()) - to_number(a->itr_fall(c.itr))));",
      to: "  v->set_fall_value(Value(to_number(v->fall_value()) - to_number(v->itr_fall(c.itr))));",
    },
    {
      note: "fall value not written",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_fall_value(Value(to_number(v->fall_value()) - to_number(a->itr_fall(c.itr))));\n",
      to: "",
    },
    {
      note: "defend value not cleared",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_defend_value(Value(0.0));\n  if (g_env.is_fall(c)) {",
      to: "  if (g_env.is_fall(c)) {",
    },
    {
      note: "defend value set to one",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_defend_value(Value(0.0));",
      to: "  v->set_defend_value(Value(1.0));",
    },
    {
      note: "impact path falls through instead of handing over",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (g_env.is_fall(c)) {\n    handle_fall(c);\n    return;\n  }",
      to: "  if (g_env.is_fall(c)) {\n    handle_fall(c);\n  }",
    },
    {
      note: "impact path hands over on a missing fall",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (g_env.is_fall(c)) {\n    handle_fall(c);\n    return;\n  }",
      to: "  if (!g_env.is_fall(c)) {\n    handle_fall(c);\n    return;\n  }",
    },
    {
      note: "impact path drops the fall handover",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (g_env.is_fall(c)) {\n    handle_fall(c);\n    return;\n  }",
      to: "  if (g_env.is_fall(c)) {\n    return;\n  }",
    },
    {
      note: "velocity y and z swapped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));",
      to: "  v->set_velocity(Value(iv.x), Value(iv.z), Value(iv.y));",
    },
    {
      note: "velocity x negated",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));",
      to: "  v->set_velocity(Value(-iv.x), Value(iv.y), Value(iv.z));",
    },
    {
      note: "velocity not written",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));\n",
      to: "",
    },
    {
      note: "spark point uses the b cube as source",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  v->spark_point(c.a_cube, c.b_cube, sx, sy, sz);",
      to: "  v->spark_point(c.b_cube, c.a_cube, sx, sy, sz);",
    },
    {
      note: "fighter test inverted",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool v_is_fighter = g_env.is_fighter(*v);",
      to: "  const bool v_is_fighter = !g_env.is_fighter(*v);",
    },
    {
      note: "bleed spark compares normal",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Sharp) && v_is_fighter) {",
      to: "  if (effect_is(effect_v, ItrEffect::Normal) && v_is_fighter) {",
    },
    {

      note: "bleed spark uses the normal hit kind",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));",
      to: "    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kHit)));",
    },
    {
      note: "hit spark uses the silent kind",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  } else if (v_is_fighter) {\n    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kHit)));",
      to: "  } else if (v_is_fighter) {\n    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kSilentHit)));",
    },
    {
      note: "hit spark uses the bleed kind",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  } else if (v_is_fighter) {\n    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kHit)));",
      to: "  } else if (v_is_fighter) {\n    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));",
    },
    {
      note: "bleed spark disabled",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool v_is_fighter = g_env.is_fighter(*v);\n  if (effect_is(effect_v, ItrEffect::Sharp) && v_is_fighter) {",
      to: "  const bool v_is_fighter = g_env.is_fighter(*v);\n  if (false && effect_is(effect_v, ItrEffect::Sharp) && v_is_fighter) {",
    },
    {
      note: "spark x taken from the y component",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));",
      to: "    g_env.spark(Value(sy), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));",
    },
    {
      note: "spark z taken from the x component",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));",
      to: "    g_env.spark(Value(sx), Value(sy), Value(sx), Value(std::u16string(spark_enum::kBleed)));",
    },
    {
      note: "caught test compares injured",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool ic = strict_equals(v->state(), Value(static_cast<double>(StateEnum::Caught)));",
      to: "  const bool ic = strict_equals(v->state(), Value(static_cast<double>(StateEnum::Injured)));",
    },
    {
      note: "caught test inverted",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool ic = strict_equals(v->state(), Value(static_cast<double>(StateEnum::Caught)));",
      to: "  const bool ic = !strict_equals(v->state(), Value(static_cast<double>(StateEnum::Caught)));",
    },
    {
      note: "hurt side comparison flipped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool on_hurt_side = v->position_y() <= v->ground_y();",
      to: "  const bool on_hurt_side = v->position_y() >= v->ground_y();",
    },
    {
      note: "hurt side comparison is strict",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool on_hurt_side = v->position_y() <= v->ground_y();",
      to: "  const bool on_hurt_side = v->position_y() < v->ground_y();",
    },
    {
      note: "hurt list alternatives swapped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const Value hurt_list = on_hurt_side ? grand_injured : injured;",
      to: "  const Value hurt_list = on_hurt_side ? injured : grand_injured;",
    },
    {
      note: "hurt list always the grand injured list",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const Value hurt_list = on_hurt_side ? grand_injured : injured;",
      to: "  const Value hurt_list = grand_injured;",
    },
    {
      note: "frame group delta adds instead of subtracting",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double r = to_number(fvm) - to_number(fv);",
      to: "  const double r = to_number(fvm) + to_number(fv);",
    },
    {
      note: "frame group delta operands swapped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double r = to_number(fvm) - to_number(fv);",
      to: "  const double r = to_number(fv) - to_number(fvm);",
    },
    {
      note: "dizzy threshold uses the critical default",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double d = kDefaultFallValueDizzy;",
      to: "  const double d = kDefaultFallValueCritical;",
    },
    {
      note: "dizzy threshold is zero",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const double d = kDefaultFallValueDizzy;",
      to: "  const double d = 0.0;",
    },
    {
      note: "dizzy comparison is strict",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  } else if (to_number(fv) <= d) {",
      to: "  } else if (to_number(fv) < d) {",
    },
    {
      note: "dizzy comparison flipped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  } else if (to_number(fv) <= d) {",
      to: "  } else if (to_number(fv) >= d) {",
    },
    {
      note: "facing comparison inverted",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool same_face = strict_equals(a->facing(), v->facing());",
      to: "  const bool same_face = !strict_equals(a->facing(), v->facing());",
    },
    {
      note: "facing comparison compares the attacker with itself",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  const bool same_face = strict_equals(a->facing(), v->facing());",
      to: "  const bool same_face = strict_equals(a->facing(), a->facing());",
    },
    {
      note: "cpoint actions swapped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = same_face ? backhurtact : fronthurtact;",
      to: "    id = same_face ? fronthurtact : backhurtact;",
    },
    {
      note: "cpoint always the back action",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = same_face ? backhurtact : fronthurtact;",
      to: "    id = backhurtact;",
    },
    {
      note: "dizzy action replaced by the front cpoint action",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = dizzy;",
      to: "    id = fronthurtact;",
    },
    {
      note: "frame group uses the raw fall value",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));",
      to: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(to_number(fv)))));",
    },
    {
      note: "frame group uses the max fall value",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));",
      to: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(to_number(fvm)))));",
    },
    {
      note: "frame group key negated",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));",
      to: "    id = index_by(hurt_list, to_string(Value(-hurt_frame_group(r))));",
    },
    {
      note: "frame group key is a constant",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));",
      to: '    id = index_by(hurt_list, u"1");',
    },
    {
      note: "frame group lookup skips the list",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));",
      to: "    id = hurt_list;",
    },
    {
      note: "length comparison is not strict",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    if (len.has_value() && *len > 0) v->enter_frame_by_id(id);",
      to: "    if (len.has_value() && *len >= 0) v->enter_frame_by_id(id);",
    },
    {
      note: "frame entered without the length test",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    if (len.has_value() && *len > 0) v->enter_frame_by_id(id);",
      to: "    (void)len;\n    v->enter_frame_by_id(id);",
    },
    {
      note: "frame entered by value instead of by id",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    if (len.has_value() && *len > 0) v->enter_frame_by_id(id);",
      to: "    if (len.has_value() && *len > 0) v->enter_frame(id);",
    },
    {
      note: "frame change never applied",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    if (len.has_value() && *len > 0) v->enter_frame_by_id(id);",
      to: "    if (len.has_value() && *len > 0) {\n      (void)id;\n    }",
    },
    {
      note: "ignore test uses strict equality",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (equals(effect_v, Value(static_cast<double>(ItrEffect::Ignore)))) return;",
      to: "  if (strict_equals(effect_v, Value(static_cast<double>(ItrEffect::Ignore)))) return;",
    },
    {
      note: "ignore effect no longer returns",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (equals(effect_v, Value(static_cast<double>(ItrEffect::Ignore)))) return;\n",
      to: "",
    },
    {
      note: "ignore comparison uses another effect",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (equals(effect_v, Value(static_cast<double>(ItrEffect::Ignore)))) return;",
      to: "  if (equals(effect_v, Value(static_cast<double>(ItrEffect::Through)))) return;",
    },
    {
      note: "armour result ignored",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (handle_armor(c)) return;",
      to: "  handle_armor(c);",
    },
    {
      note: "armour result inverted",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (handle_armor(c)) return;",
      to: "  if (!handle_armor(c)) return;",
    },
    {
      note: "fire group drops the fire effect",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire1) ||",
      to: "  if (effect_is(effect_v, ItrEffect::Ice) || effect_is(effect_v, ItrEffect::MFire1) ||",
    },
    {
      note: "fire group drops mfire2",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "      effect_is(effect_v, ItrEffect::MFire2) || effect_is(effect_v, ItrEffect::FireExplosion)) {",
      to: "      effect_is(effect_v, ItrEffect::MFire2) || false) {",
    },
    {
      note: "fire group requires both effects",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire1) ||\n      effect_is(effect_v, ItrEffect::MFire2) || effect_is(effect_v, ItrEffect::FireExplosion)) {",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) && effect_is(effect_v, ItrEffect::MFire1)) {",
    },
    {
      note: "fire group runs the second helper order",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "      effect_is(effect_v, ItrEffect::MFire2) || effect_is(effect_v, ItrEffect::FireExplosion)) {\n    apply_injury_rest_stiffness_fall(c);",
      to: "      effect_is(effect_v, ItrEffect::MFire2) || effect_is(effect_v, ItrEffect::FireExplosion)) {\n    apply_injury_stiffness_rest_fall(c);",
    },
    {
      note: "ice2 arm compares the ice effect",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Ice2)) {\n    handle_itr_effect_freeze(c);",
      to: "  if (effect_is(effect_v, ItrEffect::Ice)) {\n    handle_itr_effect_freeze(c);",
    },
    {
      note: "ice2 arm runs the fall handover",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Ice2)) {\n    handle_itr_effect_freeze(c);\n    return;\n  }",
      to: "  if (effect_is(effect_v, ItrEffect::Ice2)) {\n    handle_fall(c);\n    return;\n  }",
    },
    {
      note: "ice arm compares ice2",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Ice)) {\n    const INbdyNormalEntity* v = g_env.find_entity(c.vid);",
      to: "  if (effect_is(effect_v, ItrEffect::Ice2)) {\n    const INbdyNormalEntity* v = g_env.find_entity(c.vid);",
    },
    {
      note: "ice frozen test compares caught",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "        v != nullptr && strict_equals(v->state(), Value(static_cast<double>(StateEnum::Frozen)));",
      to: "        v != nullptr && strict_equals(v->state(), Value(static_cast<double>(StateEnum::Caught)));",
    },
    {
      note: "ice frozen test inverted",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "        v != nullptr && strict_equals(v->state(), Value(static_cast<double>(StateEnum::Frozen)));",
      to: "        v != nullptr && !strict_equals(v->state(), Value(static_cast<double>(StateEnum::Frozen)));",
    },
    {
      note: "ice frozen test dropped",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "        v != nullptr && strict_equals(v->state(), Value(static_cast<double>(StateEnum::Frozen)));",
      to: "        v != nullptr;",
    },
    {
      note: "ice arm swaps the two follow-ups",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    if (frozen) {\n      apply_injury_stiffness_rest_fall(c);\n    } else {\n      handle_itr_effect_freeze(c);\n    }",
      to: "    if (frozen) {\n      handle_itr_effect_freeze(c);\n    } else {\n      apply_injury_stiffness_rest_fall(c);\n    }",
    },
    {
      note: "impact arm drops the explosion effect",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Explosion) || effect_is(effect_v, ItrEffect::Normal) ||",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::Normal) ||",
    },
    {
      note: "impact arm drops the missing effect",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "      effect_is(effect_v, ItrEffect::Sharp) || std::holds_alternative<std::monostate>(effect_v)) {",
      to: "      effect_is(effect_v, ItrEffect::Sharp)) {",
    },
    {
      note: "impact arm requires the sharp effect only",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Explosion) || effect_is(effect_v, ItrEffect::Normal) ||\n      effect_is(effect_v, ItrEffect::Sharp) || std::holds_alternative<std::monostate>(effect_v)) {",
      to: "  if (effect_is(effect_v, ItrEffect::Sharp)) {",
    },
    {
      note: "impact arm passes the wrong effect down",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    apply_normal_impact(c, effect_v);",
      to: "    apply_normal_impact(c, Value(static_cast<double>(ItrEffect::Sharp)));",
    },
    {
      note: "impact arm never runs",
      file: "native/lfw/collision/n_bdy_normal.cpp",
      from: "    apply_normal_impact(c, effect_v);\n    return;",
      to: "    return;",
    },
  ],
};
