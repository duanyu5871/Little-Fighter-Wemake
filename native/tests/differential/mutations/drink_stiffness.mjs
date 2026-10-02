export default {
  subject: "drink_stiffness",
  mutations: [
    {
      note: "ticks_bound 的 undefined 默认值写成 0",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v)) return Times::MAX;`,
      to: `  if (std::holds_alternative<std::monostate>(v)) return 0.0;`,
    },
    {
      note: "ticks_bound 的默认值对 null 也生效",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  const Value v = field_or(info, key);
  if (std::holds_alternative<std::monostate>(v)) return Times::MAX;`,
      to: `  const Value v = field_or(info, key);
  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) {
    return Times::MAX;
  }`,
    },
    {
      note: "ticks_bound 不读实参",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  return to_number(v);
}`,
      to: `  return 0.0;
}`,
    },
    {
      note: "hp_h_ticks 的范围下界写成 1",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_ticks.set_range(0.0, ticks_bound(info, u"hp_h_ticks"));`,
      to: `  _hp_h_ticks.set_range(1.0, ticks_bound(info, u"hp_h_ticks"));`,
    },
    {
      note: "hp_r_ticks 读成 hp_h_ticks",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_r_ticks.set_range(0.0, ticks_bound(info, u"hp_r_ticks"));`,
      to: `  _hp_r_ticks.set_range(0.0, ticks_bound(info, u"hp_h_ticks"));`,
    },
    {
      note: "mp_h_ticks 读成 hp_r_ticks",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _mp_h_ticks.set_range(0.0, ticks_bound(info, u"mp_h_ticks"));`,
      to: `  _mp_h_ticks.set_range(0.0, ticks_bound(info, u"hp_r_ticks"));`,
    },
    {
      note: "hp_h_value 的默认值写成 1",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_value = or_default(field_or(info, u"hp_h_value"), Value(0.0));`,
      to: `  _hp_h_value = or_default(field_or(info, u"hp_h_value"), Value(1.0));`,
    },
    {
      note: "hp_h_value 读成 hp_r_value",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_value = or_default(field_or(info, u"hp_h_value"), Value(0.0));`,
      to: `  _hp_h_value = or_default(field_or(info, u"hp_r_value"), Value(0.0));`,
    },
    {
      note: "hp_h_total 的默认值写成 9999998",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_total = or_default(field_or(info, u"hp_h_total"), Value(9999999.0));`,
      to: `  _hp_h_total = or_default(field_or(info, u"hp_h_total"), Value(9999998.0));`,
    },
    {
      note: "hp_h_total 读成 hp_r_total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_total = or_default(field_or(info, u"hp_h_total"), Value(9999999.0));`,
      to: `  _hp_h_total = or_default(field_or(info, u"hp_r_total"), Value(9999999.0));`,
    },
    {
      note: "hp_r_value 读成 hp_h_value",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_r_value = or_default(field_or(info, u"hp_r_value"), Value(0.0));`,
      to: `  _hp_r_value = or_default(field_or(info, u"hp_h_value"), Value(0.0));`,
    },
    {
      note: "mp_h_value 的默认值写成 1",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _mp_h_value = or_default(field_or(info, u"mp_h_value"), Value(0.0));`,
      to: `  _mp_h_value = or_default(field_or(info, u"mp_h_value"), Value(1.0));`,
    },
    {
      note: "or_default 用 || 语义（吃 0/false/空串）",
      file: "native/lfw/entity/drink_info.cpp",
      from: `Value or_default(const Value& v, const Value& fallback) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v) ? fallback
                                                                                        : v;
}`,
      to: `Value or_default(const Value& v, const Value& fallback) {
  return truthy(v) ? v : fallback;
}`,
    },
    {
      note: "hp_h_empty 用 > 代替 >=",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
      to: `bool DrinkInfo::hp_h_empty() const { return gt(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
    },
    {
      note: "hp_h_empty 的值判定取反",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
      to: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || truthy(_hp_h_value); }`,
    },
    {
      note: "hp_h_empty 的第二个判定读成 total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
      to: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_total); }`,
    },
    {
      note: "hp_h_empty 的累积量读成 hp_r",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
      to: `bool DrinkInfo::hp_h_empty() const { return ge(_hp_r, _hp_h_total) || !truthy(_hp_h_value); }`,
    },
    {
      note: "hp_r_empty 读的是 hp_h 系列",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::hp_r_empty() const { return ge(_hp_r, _hp_r_total) || !truthy(_hp_r_value); }`,
      to: `bool DrinkInfo::hp_r_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }`,
    },
    {
      note: "mp_h_empty 的 total 读成 hp_h_total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::mp_h_empty() const { return ge(_mp_h, _mp_h_total) || !truthy(_mp_h_value); }`,
      to: `bool DrinkInfo::mp_h_empty() const { return ge(_mp_h, _hp_h_total) || !truthy(_mp_h_value); }`,
    },
    {
      note: "mp_h_empty 的值判定读成 mp_h_total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `bool DrinkInfo::mp_h_empty() const { return ge(_mp_h, _mp_h_total) || !truthy(_mp_h_value); }`,
      to: `bool DrinkInfo::mp_h_empty() const { return ge(_mp_h, _mp_h_total) || !truthy(_mp_h_total); }`,
    },
    {
      note: "快照的 nums 键写成 values",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  o.set(u"nums", Value(std::make_shared<lfw::Array>(arr)));`,
      to: `  o.set(u"values", Value(std::make_shared<lfw::Array>(arr)));`,
    },
    {
      note: "Times 快照的 5 个数顺序反了",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  for (double d : nums) arr.push_back(Value(d));`,
      to: `  for (std::size_t i = nums.size(); i > 0; --i) arr.push_back(Value(nums[i - 1]));`,
    },
    {
      note: "Times 快照只写 4 个数",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  for (double d : nums) arr.push_back(Value(d));`,
      to: `  for (std::size_t i = 0; i < 4; ++i) arr.push_back(Value(nums[i]));`,
    },
    {
      note: "times_read 的索引错位",
      file: "native/lfw/entity/drink_info.cpp",
      from: `    nums[i] = arr != nullptr && i < arr->size() ? to_number(arr->at(i)) : 0.0;`,
      to: `    nums[i] = arr != nullptr && i < arr->size() ? to_number(arr->at(i)) + 1.0 : 0.0;`,
    },
    {
      note: "快照的 hp_h 写的是 hp_r",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  o.set(u"hp_h", _hp_h);`,
      to: `  o.set(u"hp_h", _hp_r);`,
    },
    {
      note: "快照的 hp_h_value 写的是 hp_h_total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  o.set(u"hp_h_value", _hp_h_value);
  o.set(u"hp_h_total", _hp_h_total);`,
      to: `  o.set(u"hp_h_value", _hp_h_total);
  o.set(u"hp_h_total", _hp_h_value);`,
    },
    {
      note: "快照的 hp_h 提前到 hp_h_ticks 之前（键序错）",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  o.set(u"hp_h_ticks", times_snapshot(_hp_h_ticks));
  o.set(u"hp_h_value", _hp_h_value);`,
      to: `  o.set(u"hp_h", _hp_h);
  o.set(u"hp_h_ticks", times_snapshot(_hp_h_ticks));
  o.set(u"hp_h_value", _hp_h_value);`,
    },
    {
      note: "from_snapshot 的 hp_h_value 读成 hp_h_total",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h_value = field_or(s, u"hp_h_value");`,
      to: `  _hp_h_value = field_or(s, u"hp_h_total");`,
    },
    {
      note: "from_snapshot 的 hp_h 读成 hp_r",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_h = field_or(s, u"hp_h");`,
      to: `  _hp_h = field_or(s, u"hp_r");`,
    },
    {
      note: "from_snapshot 的 hp_r_total/hp_r_value 写反",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _hp_r_total = field_or(s, u"hp_r_total");
  _hp_r_value = field_or(s, u"hp_r_value");`,
      to: `  _hp_r_total = field_or(s, u"hp_r_value");
  _hp_r_value = field_or(s, u"hp_r_total");`,
    },
    {
      note: "from_snapshot 的 mp_h 读成 mp_h_value",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  _mp_h = field_or(s, u"mp_h");`,
      to: `  _mp_h = field_or(s, u"mp_h_value");`,
    },
    {
      note: "from_snapshot 的 mp_h_ticks 写进 hp_h_ticks",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  times_read(_mp_h_ticks, field_or(s, u"mp_h_ticks"));`,
      to: `  times_read(_hp_h_ticks, field_or(s, u"mp_h_ticks"));`,
    },
    {
      note: "from_snapshot 不读 hp_r_ticks",
      file: "native/lfw/entity/drink_info.cpp",
      from: `  times_read(_hp_r_ticks, field_or(s, u"hp_r_ticks"));`,
      to: `  if (false) times_read(_hp_r_ticks, field_or(s, u"hp_r_ticks"));`,
    },
    {
      note: "stiffness 的 Ball 判定用松相等",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  const bool is_ball =
      strict_equals(type, Value(static_cast<double>(EntityEnum::Ball)));`,
      to: `  const bool is_ball = equals(type, Value(static_cast<double>(EntityEnum::Ball)));`,
    },
    {
      note: "stiffness 的 Ball 判定写成 Fighter",
      file: "native/lfw/collision/stiffness.cpp",
      from: `      strict_equals(type, Value(static_cast<double>(EntityEnum::Ball)));`,
      to: `      strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)));`,
    },
    {
      note: "stiffness 的 type 读成顶层字段",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  const Value type = field_or(field_or(attacker, u"data"), u"type");`,
      to: `  const Value type = field_or(attacker, u"type");`,
    },
    {
      note: "stiffness 的两个 itr_motionless 键写反",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  return entity::entity_dataset(attacker,
                               is_ball ? u"ball_itr_motionless" : u"itr_motionless");`,
      to: `  return entity::entity_dataset(attacker,
                               is_ball ? u"itr_motionless" : u"ball_itr_motionless");`,
    },
    {
      note: "stiffness 的 shaking 也走 entity_dataset 链",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  return field_or(field_or(field_or(attacker, u"world"), u"dataset"), u"itr_shaking");`,
      to: `  return entity::entity_dataset(attacker, u"itr_shaking");`,
    },
    {
      note: "stiffness 的 shaking 键写成 itr_motionless",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  return field_or(field_or(field_or(attacker, u"world"), u"dataset"), u"itr_shaking");`,
      to: `  return field_or(field_or(field_or(attacker, u"world"), u"dataset"), u"itr_motionless");`,
    },
    {
      note: "stiffness 的 shaking 少读一层 world",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  return field_or(field_or(field_or(attacker, u"world"), u"dataset"), u"itr_shaking");`,
      to: `  return field_or(field_or(attacker, u"dataset"), u"itr_shaking");`,
    },
    {
      note: "stiffness 的 motionless 读成 itr.shaking",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  out.motionless = or_default(field_or(itr, u"motionless"), motionless_of(attacker));`,
      to: `  out.motionless = or_default(field_or(itr, u"shaking"), motionless_of(attacker));`,
    },
    {
      note: "stiffness 的 shaking 读成 itr.motionless",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  out.shaking = or_default(field_or(itr, u"shaking"), shaking_of(attacker));`,
      to: `  out.shaking = or_default(field_or(itr, u"motionless"), shaking_of(attacker));`,
    },
    {
      note: "stiffness 的两个兜底取值互换",
      file: "native/lfw/collision/stiffness.cpp",
      from: `  out.motionless = or_default(field_or(itr, u"motionless"), motionless_of(attacker));
  out.shaking = or_default(field_or(itr, u"shaking"), shaking_of(attacker));`,
      to: `  out.motionless = or_default(field_or(itr, u"motionless"), shaking_of(attacker));
  out.shaking = or_default(field_or(itr, u"shaking"), motionless_of(attacker));`,
    },
    {
      note: "stiffness 的 or_default 用 || 语义",
      file: "native/lfw/collision/stiffness.cpp",
      from: `Value or_default(const Value& v, const Value& fallback) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v) ? fallback
                                                                                        : v;
}`,
      to: `Value or_default(const Value& v, const Value& fallback) {
  return truthy(v) ? v : fallback;
}`,
    },
  ],
};
