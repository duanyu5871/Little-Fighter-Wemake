export default {
  subject: "collision_helpers",
  mutations: [
    {
      note: "entity_dataset 丢掉 frame.dataset 这一级",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  Value v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), name);`,
      to: `  Value v;`,
    },
    {
      note: "entity_dataset 把 data.base 提到 frame 之前",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  Value v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), name);
  if (nullish(v)) v = get_field(base, name);`,
      to: `  Value v = get_field(base, name);
  if (nullish(v)) v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), name);`,
    },
    {
      note: "entity_dataset 的 ?? 变成 ||",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}`,
      to: `bool nullish(const Value& v) { return !truthy(v); }`,
    },
    {
      note: "entity_dataset 丢掉 world.bg.data.dataset 这一级",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  if (nullish(v)) {
    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);
  }`,
      to: `  if (false) {
    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);
  }`,
    },
    {
      note: "entity_dataset 丢掉 world.dataset 这一级",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  if (nullish(v)) v = get_field(get_field(world, u"dataset"), name);`,
      to: `  if (false) v = get_field(get_field(world, u"dataset"), name);`,
    },
    {
      note: "entity_dataset 把 world.dataset 提到 bg 之前",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  if (nullish(v)) {
    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);
  }
  if (nullish(v)) v = get_field(get_field(world, u"dataset"), name);`,
      to: `  if (nullish(v)) v = get_field(get_field(world, u"dataset"), name);
  if (nullish(v)) {
    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);
  }`,
    },
    {
      note: "entity_dataset 的 bg 路径写成 world.bg.data.data",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);`,
      to: `    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"data"), name);`,
    },
    {
      note: "entity_dataset 的 bg 路径取错父节点",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `    v = get_field(get_field(get_field(get_field(world, u"bg"), u"data"), u"dataset"), name);`,
      to: `    v = get_field(get_field(get_field(get_field(world, u"data"), u"data"), u"dataset"), name);`,
    },
    {
      note: "entity_dataset 最后一级取 world.bg 而不是 world.dataset",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  if (nullish(v)) v = get_field(get_field(world, u"dataset"), name);`,
      to: `  if (nullish(v)) v = get_field(get_field(world, u"bg"), name);`,
    },
    {
      note: "entity_dataset 的 base 一级把键写死成 k",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  if (nullish(v)) v = get_field(base, name);`,
      to: `  if (nullish(v)) v = get_field(base, u"k");`,
    },
    {
      note: "entity_dataset 的 base 一级忽略 name",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  Value v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), name);`,
      to: `  Value v = get_field(get_field(get_field(entity, u"frame"), u"dataset"), u"k");`,
    },
    {
      note: "entity_dataset 把 data/base 的层级写反",
      file: "native/lfw/entity/entity_dataset.cpp",
      from: `  const Value base = get_field(get_field(entity, u"data"), u"base");`,
      to: `  const Value base = get_field(get_field(entity, u"base"), u"data");`,
    },
    {
      note: "is_fall 不检查是否 fighter",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (!entity::is_fighter(victim)) return true;`,
      to: `  if (false) return true;`,
    },
    {
      note: "is_fall 的 fighter 判定取反",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (!entity::is_fighter(victim)) return true;`,
      to: `  if (entity::is_fighter(victim)) return true;`,
    },
    {
      note: "is_fall 的 fall_value <= 0 变成 < 0",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (le(fall_value, Value(0.0))) return true;`,
      to: `  if (lt(fall_value, Value(0.0))) return true;`,
    },
    {
      note: "is_fall 第一处读的是 hp 而不是 fall_value",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  const Value fall_value = field_or(victim, u"fall_value");
  if (le(fall_value, Value(0.0))) return true;`,
      to: `  const Value fall_value = field_or(victim, u"fall_value");
  if (le(field_or(victim, u"hp"), Value(0.0))) return true;`,
    },
    {
      note: "is_fall 的 hp <= 0 变成 < 0",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (le(field_or(victim, u"hp"), Value(0.0))) return true;`,
      to: `  if (lt(field_or(victim, u"hp"), Value(0.0))) return true;`,
    },
    {
      note: "is_fall 第二处读的是 fall_value 而不是 hp",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (le(field_or(victim, u"hp"), Value(0.0))) return true;`,
      to: `  if (le(fall_value, Value(0.0))) return true;`,
    },
    {
      note: "is_fall 的 Frozen 写成 Caught",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (entity_state_is(victim, StateEnum::Frozen)) return true;`,
      to: `  if (entity_state_is(victim, StateEnum::Caught)) return true;`,
    },
    {
      note: "is_fall 的 Caught 写成 Frozen",
      file: "native/lfw/collision/is_fall.cpp",
      from: `      entity_state_is(victim, StateEnum::Caught)) {`,
      to: `      entity_state_is(victim, StateEnum::Frozen)) {`,
    },
    {
      note: "is_fall 的 state 读成顶层字段而不是 frame.state",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  return strict_equals(field_or(field_or(entity, u"frame"), u"state"),
                       Value(static_cast<double>(want)));`,
      to: `  return strict_equals(field_or(entity, u"state"), Value(static_cast<double>(want)));`,
    },
    {
      note: "is_fall 的 DIZZY 阈值用 <=",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY"))) &&`,
      to: `  if (le(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY"))) &&`,
    },
    {
      note: "is_fall 的 CRITICAL 阈值用 <=",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL"))) &&`,
      to: `  if (le(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL"))) &&`,
    },
    {
      note: "is_fall 的 DIZZY 阈值取成 CRITICAL",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY"))) &&`,
      to: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL"))) &&`,
    },
    {
      note: "is_fall 的 CRITICAL 阈值取成 DIZZY",
      file: "native/lfw/collision/is_fall.cpp",
      from: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL"))) &&`,
      to: `  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY"))) &&`,
    },
    {
      note: "is_fall 的 is_on_ground 判定取反",
      file: "native/lfw/collision/is_fall.cpp",
      from: `      !truthy(field_or(victim, u"is_on_ground"))) {`,
      to: `      truthy(field_or(victim, u"is_on_ground"))) {`,
    },
    {
      note: "is_fall 的 is_on_ground 读成 fall_value",
      file: "native/lfw/collision/is_fall.cpp",
      from: `      !truthy(field_or(victim, u"is_on_ground"))) {`,
      to: `      !truthy(field_or(victim, u"fall_value"))) {`,
    },
    {
      note: "is_armor_work 不检查护甲是否存在",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(armor)) return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "is_armor_work 的护甲存在判定取反",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(armor)) return false;`,
      to: `  if (truthy(armor)) return false;`,
    },
    {
      note: "is_armor_work 的 bframe 读成 aframe",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  const Value bframe = field_or(collision, u"bframe");`,
      to: `  const Value bframe = field_or(collision, u"aframe");`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Caught",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (state_is(bframe, StateEnum::Caught) || state_is(bframe, StateEnum::Injured) ||`,
      to: `  if (state_is(bframe, StateEnum::Walking) || state_is(bframe, StateEnum::Injured) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Injured",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (state_is(bframe, StateEnum::Caught) || state_is(bframe, StateEnum::Injured) ||`,
      to: `  if (state_is(bframe, StateEnum::Caught) || state_is(bframe, StateEnum::Walking) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Falling",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::Falling) || state_is(bframe, StateEnum::Frozen) ||`,
      to: `state_is(bframe, StateEnum::Walking) || state_is(bframe, StateEnum::Frozen) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Frozen",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::Frozen) ||
      state_is(bframe, StateEnum::Lying) ||`,
      to: `state_is(bframe, StateEnum::Walking) ||
      state_is(bframe, StateEnum::Lying) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Lying",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::Lying) || state_is(bframe, StateEnum::Tired) ||`,
      to: `state_is(bframe, StateEnum::Walking) || state_is(bframe, StateEnum::Tired) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Tired",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::Tired) ||
      state_is(bframe, StateEnum::BrokenDefend) ||`,
      to: `state_is(bframe, StateEnum::Walking) ||
      state_is(bframe, StateEnum::BrokenDefend) ||`,
    },
    {
      note: "is_armor_work 受击帧漏掉 BrokenDefend",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::BrokenDefend) || state_is(bframe, StateEnum::Burning)) {`,
      to: `state_is(bframe, StateEnum::Walking) || state_is(bframe, StateEnum::Burning)) {`,
    },
    {
      note: "is_armor_work 受击帧漏掉 Burning",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `state_is(bframe, StateEnum::BrokenDefend) || state_is(bframe, StateEnum::Burning)) {`,
      to: `state_is(bframe, StateEnum::BrokenDefend) || state_is(bframe, StateEnum::Walking)) {`,
    },
    {
      note: "is_armor_work 的后半段列表漏掉 Dash",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
      to: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d);`,
    },
    {
      note: "is_armor_work 的后半段列表漏掉 Standing",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
      to: `  return state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
    },
    {
      note: "is_armor_work 的后半段列表漏掉 Walking",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
      to: `  return state_is(v, a) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
    },
    {
      note: "is_armor_work 的后半段列表漏掉 Running",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
      to: `  return state_is(v, a) || state_is(v, b) || state_is(v, d) || state_is(v, e);`,
    },
    {
      note: "is_armor_work 的后半段列表漏掉 Jump",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, d) || state_is(v, e);`,
      to: `  return state_is(v, a) || state_is(v, b) || state_is(v, c) || state_is(v, e);`,
    },
    {
      note: "is_armor_work 的后半段列表把 Dash 当成 Rowing",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      !state_in_any(bframe, StateEnum::Standing, StateEnum::Walking, StateEnum::Running,
                    StateEnum::Jump, StateEnum::Dash)) {`,
      to: `      !state_in_any(bframe, StateEnum::Standing, StateEnum::Walking, StateEnum::Running,
                    StateEnum::Jump, StateEnum::Rowing)) {`,
    },
    {
      note: "is_armor_work 的后半段列表把 Standing 当成 Rowing",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      !state_in_any(bframe, StateEnum::Standing, StateEnum::Walking, StateEnum::Running,`,
      to: `      !state_in_any(bframe, StateEnum::Rowing, StateEnum::Walking, StateEnum::Running,`,
    },
    {
      note: "is_armor_work 的非全时限制取反",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      !state_in_any(bframe, StateEnum::Standing, StateEnum::Walking, StateEnum::Running,`,
      to: `      state_in_any(bframe, StateEnum::Standing, StateEnum::Walking, StateEnum::Running,`,
    },
    {
      note: "is_armor_work 的 fulltime === false 变成 === 0",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (strict_equals(field_or(armor, u"fulltime"), Value(false)) &&`,
      to: `  if (strict_equals(field_or(armor, u"fulltime"), Value(0.0)) &&`,
    },
    {
      note: "is_armor_work 的 fulltime 用松相等",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (strict_equals(field_or(armor, u"fulltime"), Value(false)) &&`,
      to: `  if (equals(field_or(armor, u"fulltime"), Value(false)) &&`,
    },
    {
      note: "is_armor_work 的 fulltime 读成 fireproof",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (strict_equals(field_or(armor, u"fulltime"), Value(false)) &&`,
      to: `  if (strict_equals(field_or(armor, u"fireproof"), Value(false)) &&`,
    },
    {
      note: "is_armor_work 不检查防火",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"fireproof")) &&`,
      to: `  if (false &&`,
    },
    {
      note: "is_armor_work 的 fireproof 判定取反",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"fireproof")) &&`,
      to: `  if (truthy(field_or(armor, u"fireproof")) &&`,
    },
    {
      note: "is_armor_work 的 fireproof 读成 antifreeze",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"fireproof")) &&`,
      to: `  if (!truthy(field_or(armor, u"antifreeze")) &&`,
    },
    {
      note: "is_armor_work 的火系列表漏掉 Fire",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      (effect_is(effect, ItrEffect::Fire) || effect_is(effect, ItrEffect::MFire1) ||`,
      to: `      (effect_is(effect, ItrEffect::Ice) || effect_is(effect, ItrEffect::MFire1) ||`,
    },
    {
      note: "is_armor_work 的火系列表漏掉 MFire1",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      (effect_is(effect, ItrEffect::Fire) || effect_is(effect, ItrEffect::MFire1) ||`,
      to: `      (effect_is(effect, ItrEffect::Fire) || effect_is(effect, ItrEffect::Ice) ||`,
    },
    {
      note: "is_armor_work 的火系列表漏掉 MFire2",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `       effect_is(effect, ItrEffect::MFire2) || effect_is(effect, ItrEffect::FireExplosion))) {`,
      to: `       effect_is(effect, ItrEffect::Ice) || effect_is(effect, ItrEffect::FireExplosion))) {`,
    },
    {
      note: "is_armor_work 的火系列表把 FireExplosion 当成 Explosion",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `       effect_is(effect, ItrEffect::MFire2) || effect_is(effect, ItrEffect::FireExplosion))) {`,
      to: `       effect_is(effect, ItrEffect::MFire2) || effect_is(effect, ItrEffect::Ice2))) {`,
    },
    {
      note: "is_armor_work 不检查防冰",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"antifreeze")) &&`,
      to: `  if (false &&`,
    },
    {
      note: "is_armor_work 的 antifreeze 判定取反",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"antifreeze")) &&`,
      to: `  if (truthy(field_or(armor, u"antifreeze")) &&`,
    },
    {
      note: "is_armor_work 的 antifreeze 读成 fireproof",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (!truthy(field_or(armor, u"antifreeze")) &&`,
      to: `  if (!truthy(field_or(armor, u"fireproof")) &&`,
    },
    {
      note: "is_armor_work 的冰系列表漏掉 Ice2",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      (effect_is(effect, ItrEffect::Ice2) || effect_is(effect, ItrEffect::Ice))) {`,
      to: `      (effect_is(effect, ItrEffect::Fire) || effect_is(effect, ItrEffect::Ice))) {`,
    },
    {
      note: "is_armor_work 的冰系列表漏掉 Ice",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `      (effect_is(effect, ItrEffect::Ice2) || effect_is(effect, ItrEffect::Ice))) {`,
      to: `      (effect_is(effect, ItrEffect::Ice2) || effect_is(effect, ItrEffect::Fire))) {`,
    },
    {
      note: "is_armor_work 的破防值用 >",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (ge(field_or(itr, u"bdefend"),`,
      to: `  if (gt(field_or(itr, u"bdefend"),`,
    },
    {
      note: "is_armor_work 的破防值用 <=",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (ge(field_or(itr, u"bdefend"),`,
      to: `  if (le(field_or(itr, u"bdefend"),`,
    },
    {
      note: "is_armor_work 的 bdefend 读成 injury",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (ge(field_or(itr, u"bdefend"),`,
      to: `  if (ge(field_or(itr, u"injury"),`,
    },
    {
      note: "is_armor_work 的破防阈值取成 DIZZY",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `         Value(defines::num(u"Defines.DEFAULT_FORCE_BREAK_DEFEND_VALUE")))) {`,
      to: `         Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY")))) {`,
    },
    {
      note: "is_armor_work 的 aframe 读成 bframe",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (state_is(field_or(collision, u"aframe"), StateEnum::Ball_3006)) return false;`,
      to: `  if (state_is(field_or(collision, u"bframe"), StateEnum::Ball_3006)) return false;`,
    },
    {
      note: "is_armor_work 的 Ball_3006 写成 Frozen",
      file: "native/lfw/collision/is_armor_work.cpp",
      from: `  if (state_is(field_or(collision, u"aframe"), StateEnum::Ball_3006)) return false;`,
      to: `  if (state_is(field_or(collision, u"aframe"), StateEnum::Frozen)) return false;`,
    },
    {
      note: "calc_itr_velocity 的 dvx 丢掉默认值 0",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double dvx = number_or(field_or(itr, u"dvx"), 0.0);`,
      to: `  const double dvx = to_number(field_or(itr, u"dvx"));`,
    },
    {
      note: "calc_itr_velocity 的 dvz 丢掉默认值 0",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double dvz = number_or(field_or(itr, u"dvz"), 0.0);`,
      to: `  const double dvz = to_number(field_or(itr, u"dvz"));`,
    },
    {
      note: "calc_itr_velocity 的 dvy 丢掉默认值",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double dvy = std::holds_alternative<std::monostate>(dvy_raw)
                         ? to_number(entity::entity_dataset(attacker, u"ivy_d"))
                         : to_number(dvy_raw);`,
      to: `  const double dvy = to_number(dvy_raw);`,
    },
    {
      note: "calc_itr_velocity 的 dvy 默认键取成 ivy_f",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `                         ? to_number(entity::entity_dataset(attacker, u"ivy_d"))`,
      to: `                         ? to_number(entity::entity_dataset(attacker, u"ivy_f"))`,
    },
    {
      note: "calc_itr_velocity 的 dvy 默认值对 null 也生效",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double dvy = std::holds_alternative<std::monostate>(dvy_raw)`,
      to: `  const double dvy = (std::holds_alternative<std::monostate>(dvy_raw) ||
                      std::holds_alternative<NullTag>(dvy_raw))`,
    },
    {
      note: "calc_itr_velocity 的 x 差变成加法",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double diff_x = x_of(victim) - x_of(attacker);`,
      to: `  const double diff_x = x_of(victim) + x_of(attacker);`,
    },
    {
      note: "calc_itr_velocity 的 x 差方向反了",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const double diff_x = x_of(victim) - x_of(attacker);`,
      to: `  const double diff_x = x_of(attacker) - x_of(victim);`,
    },
    {
      note: "calc_itr_velocity 的 x 读成 z",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `double x_of(const Value& entity) {
  return to_number(field_or(field_or(entity, u"position"), u"x"));
}`,
      to: `double x_of(const Value& entity) {
  return to_number(field_or(field_or(entity, u"position"), u"z"));
}`,
    },
    {
      note: "calc_itr_velocity 的 FireExplosion 判定用严相等",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::FireExplosion))) ||`,
      to: `      strict_equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::FireExplosion))) ||`,
    },
    {
      note: "calc_itr_velocity 漏掉 Explosion 判定",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::Explosion))) ||`,
      to: `      equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::Fire))) ||`,
    },
    {
      note: "calc_itr_velocity 的 HeavyWeapon_InTheSky 判定用松相等",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      strict_equals(field_or(field_or(attacker, u"frame"), u"state"),`,
      to: `      equals(field_or(field_or(attacker, u"frame"), u"state"),`,
    },
    {
      note: "calc_itr_velocity 的 HeavyWeapon_InTheSky 写成 OnHand",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `                    Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)));`,
      to: `                    Value(static_cast<double>(StateEnum::HeavyWeapon_OnHand)));`,
    },
    {
      note: "calc_itr_velocity 的 attacker 状态读成顶层字段",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      strict_equals(field_or(field_or(attacker, u"frame"), u"state"),`,
      to: `      strict_equals(field_or(attacker, u"state"),`,
    },
    {
      note: "calc_itr_velocity 的 x_direction 被提前转成数字",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `    x_direction = field_or(attacker, u"facing");`,
      to: `    x_direction = Value(to_number(field_or(attacker, u"facing")));`,
    },
    {
      note: "calc_itr_velocity 的 x_direction 恒为 1",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `    x_direction = field_or(attacker, u"facing");`,
      to: `    x_direction = Value(1.0);`,
    },
    {
      note: "calc_itr_velocity 的 position_based 分支取反",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  if (!position_based) {`,
      to: `  if (position_based) {`,
    },
    {
      note: "calc_itr_velocity 的 diff_x < 0 分支方向写错",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  } else if (diff_x < 0.0) {
    x_direction = Value(1.0);
  }`,
      to: `  } else if (diff_x < 0.0) {
    x_direction = Value(-1.0);
  }`,
    },
    {
      note: "calc_itr_velocity 的 diff_x > 0 分支方向写错",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  } else if (diff_x > 0.0) {
    x_direction = Value(-1.0);
  }`,
      to: `  } else if (diff_x > 0.0) {
    x_direction = Value(1.0);
  }`,
    },
    {
      note: "calc_itr_velocity 的 x_direction 初值写成 1",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  Value x_direction(-1.0);`,
      to: `  Value x_direction(1.0);`,
    },
    {
      note: "calc_itr_velocity 的 weight 默认值写成 0",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  if (std::holds_alternative<std::monostate>(w) || std::holds_alternative<NullTag>(w)) return 1.0;`,
      to: `  if (std::holds_alternative<std::monostate>(w) || std::holds_alternative<NullTag>(w)) return 0.0;`,
    },
    {
      note: "calc_itr_velocity 的 weight 用 || 而不是 ??",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  if (std::holds_alternative<std::monostate>(w) || std::holds_alternative<NullTag>(w)) return 1.0;`,
      to: `  if (!truthy(w)) return 1.0;`,
    },
    {
      note: "calc_itr_velocity 的 weight 读成顶层字段",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  const Value w = field_or(field_or(field_or(victim, u"data"), u"base"), u"weight");`,
      to: `  const Value w = field_or(victim, u"weight");`,
    },
    {
      note: "calc_itr_velocity 的 y 项不看 is_fall",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      (is_fall(collision) ? dvy * to_number(entity::entity_dataset(attacker, u"ivy_f")) : 0.0) /`,
      to: `      (dvy * to_number(entity::entity_dataset(attacker, u"ivy_f"))) /`,
    },
    {
      note: "calc_itr_velocity 的 y 项不乘 ivy_f",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      (is_fall(collision) ? dvy * to_number(entity::entity_dataset(attacker, u"ivy_f")) : 0.0) /`,
      to: `      (is_fall(collision) ? dvy : 0.0) /`,
    },
    {
      note: "calc_itr_velocity 的 y 项缩放键取成 ivz_f",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `      (is_fall(collision) ? dvy * to_number(entity::entity_dataset(attacker, u"ivy_f")) : 0.0) /`,
      to: `      (is_fall(collision) ? dvy * to_number(entity::entity_dataset(attacker, u"ivz_f")) : 0.0) /`,
    },
    {
      note: "calc_itr_velocity 的 x 缩放键取成 ivy_f",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /`,
      to: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivy_f")) * to_number(x_direction) /`,
    },
    {
      note: "calc_itr_velocity 的 z 缩放键取成 ivx_f",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.z = dvz * to_number(entity::entity_dataset(attacker, u"ivz_f")) / weight;`,
      to: `  out.z = dvz * to_number(entity::entity_dataset(attacker, u"ivx_f")) / weight;`,
    },
    {
      note: "calc_itr_velocity 的 x 项忽略 x_direction",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /
          weight;`,
      to: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) / weight;`,
    },
    {
      note: "calc_itr_velocity 的 x 项用 dvy",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /`,
      to: `  out.x = dvy * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /`,
    },
    {
      note: "calc_itr_velocity 的 x 项乘 weight",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /
          weight;`,
      to: `  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) *
          weight;`,
    },
    {
      note: "calc_itr_velocity 的 z 项乘 weight",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.z = dvz * to_number(entity::entity_dataset(attacker, u"ivz_f")) / weight;`,
      to: `  out.z = dvz * to_number(entity::entity_dataset(attacker, u"ivz_f")) * weight;`,
    },
    {
      note: "calc_itr_velocity 的 x_direction 输出恒为 0",
      file: "native/lfw/collision/calc_itr_velocity.cpp",
      from: `  out.x_direction = x_direction;`,
      to: `  out.x_direction = Value(0.0);`,
    },
  ],
};
