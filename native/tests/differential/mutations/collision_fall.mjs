/**
 * Mutation spec for `native/lfw/collision/fall.cpp`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `if (v == nullptr) return;` -> dropped line, or inverted to `!=`. The harness
 *    always registers both ids (`A` and `V`), so the lookup can never return null
 *    and the variants are indistinguishable. The guard is a port artefact: the TS
 *    original destructures `collision.victim`.
 * 2. `truthy(fall) && ge(fall, Defines.DEFAULT_FALL_VALUE_CRITICAL)` -> dropping
 *    the `truthy(fall) &&` prefix. No JS value is falsy while also satisfying
 *    `value >= 100`: `>=` forces `ToNumber`, every numeric result `>= 100` is
 *    non-zero, and every other falsy value coerces below 100 or to `NaN` (and the
 *    port's `ge` returns false when `ToNumber` yields `NaN`). Dead in both.
 * 3. `const char16_t* effect = spark_enum::kHit;` -> any other initial value. The
 *    following `if / else if / else if / else` chain is exhaustive, so the
 *    initialiser is overwritten on every path.
 * 4. `index_by`'s bounds guard `if (!(index >= 0) || index >= size) return Value();`.
 *    The only keys ever looked up are `"1"` and `"-1"`: for `"1"` both sides of a
 *    changed bound still admit the lookup, and for `"-1"` both sides still reject
 *    it. Widening the guard instead (`index >= size` -> more permissive) would make
 *    the `"-1"` path call `Array::at` with a negative index, i.e. undefined
 *    behaviour rather than a defined divergence, so it is excluded on purpose.
 * 5. `if (arr->size() == 0) return Value();` inside `index_0` -> changing the
 *    comparison bound. Any such change reads `Array::at(0)` out of bounds for the
 *    empty-list cases, i.e. undefined behaviour rather than a defined divergence.
 * 6. `v->velocity_x() / to_number(v->facing())` -> `v->velocity_x() *
 *    to_number(v->facing())`. `x / f` and `x * f` always have the same sign for the
 *    only two facings the case file uses (`1` and `-1`), so no case can separate
 *    them.
 *
 * Deliberate divergence from the TS original: `critical_hit[direction]` throws a
 * `TypeError` when the key is missing, which aborts collision resolution. The port
 * must not throw, so `index_by` returns `undefined` instead. The case file
 * therefore never omits a direction key (that TS-side crash is not observable as a
 * differential line), and the port's non-throwing choice is recorded here.
 */
export default {
  subject: "collision_fall",
  mutations: [
    {
      note: "index_by: object branch disabled",
      file: "native/lfw/collision/fall.cpp",
      from: "  const Object* o = as_object(holder);\n  if (o != nullptr) {\n    const Value* p = o->get(k);\n    if (p != nullptr) return *p;\n  }",
      to: "  const Object* o = nullptr;\n  if (o != nullptr) {\n    const Value* p = o->get(k);\n    if (p != nullptr) return *p;\n  }",
    },
    {
      note: "index_by: array index taken from the wrong element",
      file: "native/lfw/collision/fall.cpp",
      from: "    return arr->at(static_cast<size_t>(index));",
      to: "    return arr->at(0);",
    },
    {
      note: "index_0: reads the second array element",
      file: "native/lfw/collision/fall.cpp",
      from: "    return arr->at(0);",
      to: "    return arr->at(1);",
    },
    {
      note: "index_0: reads the keyed object from key 1",
      file: "native/lfw/collision/fall.cpp",
      from: '    const Value* p = o->get(u"0");',
      to: '    const Value* p = o->get(u"1");',
    },
    {
      note: "index_0: array branch disabled",
      file: "native/lfw/collision/fall.cpp",
      from: "  const Array* arr = as_array(holder);\n  if (arr != nullptr) {\n    if (arr->size() == 0) return Value();\n    return arr->at(0);\n  }",
      to: "  const Array* arr = nullptr;\n  if (arr != nullptr) {\n    if (arr->size() == 0) return Value();\n    return arr->at(0);\n  }",
    },
    {
      note: "effect_is: compares against a shifted enum",
      file: "native/lfw/collision/fall.cpp",
      from: "  return strict_equals(effect, Value(static_cast<double>(want)));",
      to: "  return strict_equals(effect, Value(static_cast<double>(want) + 1.0));",
    },
    {
      note: "victim resolved from the attacker id",
      file: "native/lfw/collision/fall.cpp",
      from: "  IFallEntity* v = g_env.find_entity(c.vid);",
      to: "  IFallEntity* v = g_env.find_entity(c.aid);",
    },
    {
      note: "toughness not zeroed",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_toughness(Value(0.0));\n  v->set_fall_value(Value(0.0));",
      to: "  v->set_fall_value(Value(0.0));",
    },
    {
      note: "toughness set to one",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_toughness(Value(0.0));",
      to: "  v->set_toughness(Value(1.0));",
    },
    {
      note: "fall value not zeroed",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_fall_value(Value(0.0));\n  v->set_defend_value(Value(0.0));",
      to: "  v->set_defend_value(Value(0.0));",
    },
    {
      note: "fall value set to one",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_fall_value(Value(0.0));",
      to: "  v->set_fall_value(Value(1.0));",
    },
    {
      note: "defend value not zeroed",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_defend_value(Value(0.0));\n  v->set_resting(Value(0.0));",
      to: "  v->set_resting(Value(0.0));",
    },
    {
      note: "defend value set to one",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_defend_value(Value(0.0));",
      to: "  v->set_defend_value(Value(1.0));",
    },
    {
      note: "resting not zeroed",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_resting(Value(0.0));\n\n  const ItrVelocity iv = g_env.calc_velocity(c);",
      to: "  const ItrVelocity iv = g_env.calc_velocity(c);",
    },
    {
      note: "resting set to one",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_resting(Value(0.0));",
      to: "  v->set_resting(Value(1.0));",
    },
    {
      note: "resting zeroed before the defend value",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_defend_value(Value(0.0));\n  v->set_resting(Value(0.0));",
      to: "  v->set_resting(Value(0.0));\n  v->set_defend_value(Value(0.0));",
    },
    {
      note: "velocity not written",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));\n",
      to: "",
    },
    {
      note: "velocity x negated",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));",
      to: "  v->set_velocity(Value(-iv.x), Value(iv.y), Value(iv.z));",
    },
    {
      note: "velocity y and z swapped",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));",
      to: "  v->set_velocity(Value(iv.x), Value(iv.z), Value(iv.y));",
    },
    {
      note: "velocity y taken from x",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));",
      to: "  v->set_velocity(Value(iv.x), Value(iv.x), Value(iv.z));",
    },
    {
      note: "fall key misspelled",
      file: "native/lfw/collision/fall.cpp",
      from: '  const Value fall = field_or(c.itr, u"fall");',
      to: '  const Value fall = field_or(c.itr, u"falls");',
    },
    {
      note: "critical test uses or",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) || ge(fall, Value(kDefaultFallValueCritical));",
    },
    {
      note: "critical comparison is strict",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) && gt(fall, Value(kDefaultFallValueCritical));",
    },
    {
      note: "critical comparison flipped",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) && le(fall, Value(kDefaultFallValueCritical));",
    },
    {
      note: "critical comparison operands swapped",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) && ge(Value(kDefaultFallValueCritical), fall);",
    },
    {
      note: "critical threshold uses the dizzy default",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueDizzy));",
    },
    {
      note: "critical threshold is zero",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_critical = truthy(fall) && ge(fall, Value(kDefaultFallValueCritical));",
      to: "  const bool is_critical = truthy(fall) && ge(fall, Value(0.0));",
    },
    {
      note: "spark point uses the b cube as source",
      file: "native/lfw/collision/fall.cpp",
      from: "  v->spark_point(c.a_cube, c.b_cube, sx, sy, sz);",
      to: "  v->spark_point(c.b_cube, c.a_cube, sx, sy, sz);",
    },
    {
      note: "fighter test inverted",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool v_is_fighter = g_env.is_fighter(*v);",
      to: "  const bool v_is_fighter = !g_env.is_fighter(*v);",
    },
    {
      note: "effect key misspelled",
      file: "native/lfw/collision/fall.cpp",
      from: '  const Value effect_v = field_or(c.itr, u"effect");',
      to: '  const Value effect_v = field_or(c.itr, u"effects");',
    },
    {
      note: "sharp test compares fire",
      file: "native/lfw/collision/fall.cpp",
      from: "  const bool is_sharp = effect_is(effect_v, ItrEffect::Sharp);",
      to: "  const bool is_sharp = effect_is(effect_v, ItrEffect::Fire);",
    },
    {
      note: "critical bleed guard drops the fighter test",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (v_is_fighter && is_sharp && is_critical) {",
      to: "  if (is_sharp && is_critical) {",
    },
    {
      note: "critical bleed guard drops the sharp test",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (v_is_fighter && is_sharp && is_critical) {",
      to: "  if (v_is_fighter && is_critical) {",
    },
    {
      note: "critical bleed guard drops the critical test",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (v_is_fighter && is_sharp && is_critical) {",
      to: "  if (v_is_fighter && is_sharp) {",
    },
    {
      note: "critical bleed guard uses or",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (v_is_fighter && is_sharp && is_critical) {",
      to: "  if (v_is_fighter && is_sharp || is_critical) {",
    },
    {
      note: "critical bleed kind swapped for the bleed fall",
      file: "native/lfw/collision/fall.cpp",
      from: "    effect = spark_enum::kCriticalBleed;",
      to: "    effect = spark_enum::kBleedFall;",
    },
    {
      note: "bleed fall guard drops the fighter test",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (v_is_fighter && is_sharp) {",
      to: "  } else if (is_sharp) {",
    },
    {
      note: "bleed fall guard drops the sharp test",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (v_is_fighter && is_sharp) {",
      to: "  } else if (v_is_fighter) {",
    },
    {
      note: "bleed fall kind swapped for the critical bleed",
      file: "native/lfw/collision/fall.cpp",
      from: "    effect = spark_enum::kBleedFall;",
      to: "    effect = spark_enum::kCriticalBleed;",
    },
    {
      note: "critical hit guard negated",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (is_critical) {",
      to: "  } else if (!is_critical) {",
    },
    {
      note: "critical hit kind swapped for the silent variant",
      file: "native/lfw/collision/fall.cpp",
      from: "    effect = spark_enum::kCriticalHit;",
      to: "    effect = spark_enum::kSilentCriticalHit;",
    },
    {
      note: "hit fall kind swapped for the standing hit",
      file: "native/lfw/collision/fall.cpp",
      from: "    effect = spark_enum::kHitFall;",
      to: "    effect = spark_enum::kHit;",
    },
    {
      note: "spark x taken from the y component",
      file: "native/lfw/collision/fall.cpp",
      from: "  g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(effect)));",
      to: "  g_env.spark(Value(sy), Value(sy), Value(sz), Value(std::u16string(effect)));",
    },
    {
      note: "spark y taken from the z component",
      file: "native/lfw/collision/fall.cpp",
      from: "  g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(effect)));",
      to: "  g_env.spark(Value(sx), Value(sz), Value(sz), Value(std::u16string(effect)));",
    },
    {
      note: "spark kind is a constant",
      file: "native/lfw/collision/fall.cpp",
      from: "  g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(effect)));",
      to: "  g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kHit)));",
    },
    {
      note: "spark never emitted",
      file: "native/lfw/collision/fall.cpp",
      from: "  g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(effect)));\n",
      to: "",
    },
    {
      note: "fire list read from the critical list slot",
      file: "native/lfw/collision/fall.cpp",
      from: "  const Value fire = v->data_indexes_fire();",
      to: "  const Value fire = v->data_indexes_critical_hit();",
    },
    {
      note: "critical list read from the fire slot",
      file: "native/lfw/collision/fall.cpp",
      from: "  const Value critical_hit = v->data_indexes_critical_hit();",
      to: "  const Value critical_hit = v->data_indexes_fire();",
    },
    {
      note: "critical list guard dropped",
      file: "native/lfw/collision/fall.cpp",
      from: "    if (!truthy(critical_hit)) return;\n",
      to: "",
    },
    {
      note: "critical list guard inverted",
      file: "native/lfw/collision/fall.cpp",
      from: "    if (!truthy(critical_hit)) return;",
      to: "    if (truthy(critical_hit)) return;",
    },
    {
      note: "direction comparison is strict",
      file: "native/lfw/collision/fall.cpp",
      from: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
      to: "    const double direction = v->velocity_x() / to_number(v->facing()) > 0 ? 1.0 : -1.0;",
    },
    {
      note: "direction compares the facing only",
      file: "native/lfw/collision/fall.cpp",
      from: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
      to: "    const double direction = v->velocity_x() / 1.0 >= 0 ? 1.0 : -1.0;",
    },
    {
      note: "direction numerator is a constant",
      file: "native/lfw/collision/fall.cpp",
      from: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
      to: "    const double direction = 1.0 / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
    },
    {
      note: "positive direction is two",
      file: "native/lfw/collision/fall.cpp",
      from: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
      to: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 2.0 : -1.0;",
    },
    {
      note: "negative direction is minus two",
      file: "native/lfw/collision/fall.cpp",
      from: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -1.0;",
      to: "    const double direction = v->velocity_x() / to_number(v->facing()) >= 0 ? 1.0 : -2.0;",
    },
    {
      note: "direction key is always positive",
      file: "native/lfw/collision/fall.cpp",
      from: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(direction)))));",
      to: '    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(1.0)))));',
    },
    {
      note: "direction key negated",
      file: "native/lfw/collision/fall.cpp",
      from: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(direction)))));",
      to: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(-direction)))));",
    },
    {
      note: "critical frame lookup skips the direction",
      file: "native/lfw/collision/fall.cpp",
      from: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(direction)))));",
      to: "    v->enter_frame_by_id(index_0(critical_hit));",
    },
    {
      note: "first-element lookup dropped",
      file: "native/lfw/collision/fall.cpp",
      from: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(direction)))));",
      to: "    v->enter_frame_by_id(index_by(critical_hit, to_string(Value(direction))));",
    },
    {
      note: "critical frame never entered",
      file: "native/lfw/collision/fall.cpp",
      from: "    v->enter_frame_by_id(index_0(index_by(critical_hit, to_string(Value(direction)))));\n",
      to: "",
    },
    {
      note: "fire branch uses and",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) && effect_is(effect_v, ItrEffect::MFire2)) {",
    },
    {
      note: "fire branch compares the ice effect",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {",
      to: "  if (effect_is(effect_v, ItrEffect::Ice) || effect_is(effect_v, ItrEffect::MFire2)) {",
    },
    {
      note: "fire branch compares mfire1",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire1)) {",
    },
    {
      note: "mfire1 branch uses and",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {",
      to: "  } else if (effect_is(effect_v, ItrEffect::MFire1) &&\n             effect_is(effect_v, ItrEffect::FireExplosion)) {",
    },
    {
      note: "mfire1 branch compares the fire effect",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {",
      to: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::Fire)) {",
    },
    {
      note: "fire branch never enters the frame",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));\n      v->enter_frame(Value(info));',
      to: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
    },
    {
      note: "fire branch frame id key misspelled",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
      to: '      info->set(u"ids", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
    },
    {
      note: "fire branch facing key misspelled",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
      to: '      info->set(u"id", index_0(fire));\n      info->set(u"facings", entity::turn_face(iv.x_direction));',
    },
    {
      note: "fire branch frame id is a constant",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
      to: '      info->set(u"id", Value(u"x"));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
    },
    {
      note: "fire branch facing is not turned",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"facing", entity::turn_face(iv.x_direction));',
      to: '      info->set(u"facing", iv.x_direction);',
    },
    {
      note: "fire branch keys written in the other order",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", entity::turn_face(iv.x_direction));',
      to: '      info->set(u"facing", entity::turn_face(iv.x_direction));\n      info->set(u"id", index_0(fire));',
    },
    {
      note: "mfire1 branch facing is turned",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"facing", iv.x_direction);',
      to: '      info->set(u"facing", entity::turn_face(iv.x_direction));',
    },
    {
      note: "fire branch holding test inverted",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (!strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
    },
    {
      note: "fire branch holding test compares a knife",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Knife)))) {\n        v->drop_holding();\n      }",
    },
    {
      note: "fire branch never drops the holding",
      file: "native/lfw/collision/fall.cpp",
      from: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
      to: "  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire2)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n      }",
    },
    {
      note: "fire branch truthiness inverted",
      file: "native/lfw/collision/fall.cpp",
      from: "    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }\n      auto info = std::make_shared<Object>();\n      info->set(u\"id\", index_0(fire));\n      info->set(u\"facing\", entity::turn_face(iv.x_direction));",
      to: "    if (!truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }\n      auto info = std::make_shared<Object>();\n      info->set(u\"id\", index_0(fire));\n      info->set(u\"facing\", entity::turn_face(iv.x_direction));",
    },
    {
      note: "mfire1 branch holding test inverted",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
      to: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {\n    if (truthy(fire)) {\n      if (!strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
    },
    {
      note: "mfire1 branch never drops the holding",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n        v->drop_holding();\n      }",
      to: "  } else if (effect_is(effect_v, ItrEffect::MFire1) ||\n             effect_is(effect_v, ItrEffect::FireExplosion)) {\n    if (truthy(fire)) {\n      if (strict_equals(v->holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {\n      }",
    },
    {
      note: "mfire1 branch frame id is a constant",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", iv.x_direction);\n      v->enter_frame(Value(info));',
      to: '      info->set(u"id", Value(u"x"));\n      info->set(u"facing", iv.x_direction);\n      v->enter_frame(Value(info));',
    },
    {
      note: "mfire1 branch frame id key misspelled",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", iv.x_direction);',
      to: '      info->set(u"ids", index_0(fire));\n      info->set(u"facing", iv.x_direction);',
    },
    {
      note: "mfire1 branch never enters the frame",
      file: "native/lfw/collision/fall.cpp",
      from: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", iv.x_direction);\n      v->enter_frame(Value(info));',
      to: '      info->set(u"id", index_0(fire));\n      info->set(u"facing", iv.x_direction);',
    },
    {
      note: "fire fallback skipped",
      file: "native/lfw/collision/fall.cpp",
      from: "      v->enter_frame(Value(info));\n    } else {\n      normal_fall_act();\n    }\n  } else if (effect_is(effect_v, ItrEffect::MFire1) ||",
      to: "      v->enter_frame(Value(info));\n    }\n  } else if (effect_is(effect_v, ItrEffect::MFire1) ||",
    },
    {
      note: "mfire1 fallback skipped",
      file: "native/lfw/collision/fall.cpp",
      from: "      v->enter_frame(Value(info));\n    } else {\n      normal_fall_act();\n    }\n  } else {\n    normal_fall_act();\n  }",
      to: "      v->enter_frame(Value(info));\n    }\n  } else {\n    normal_fall_act();\n  }",
    },
    {
      note: "default fallback skipped",
      file: "native/lfw/collision/fall.cpp",
      from: "  } else {\n    normal_fall_act();\n  }\n}",
      to: "  }\n}",
    },
  ],
};
