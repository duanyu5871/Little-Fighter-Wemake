export default {
  subject: "entity_helpers",
  mutations: [
    {
      note: "calc_v 的 Fixed 模式返回 current",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::Fixed)) return value;`,
      to: `  if (m == static_cast<double>(SpeedMode::Fixed)) return current;`,
    },
    {
      note: "calc_v 的 Extra 模式返回 value",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::Extra)) return current;`,
      to: `  if (m == static_cast<double>(SpeedMode::Extra)) return value;`,
    },
    {
      note: "calc_v 的 FixedAcc 模式用减法",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::FixedAcc)) return current + value;`,
      to: `  if (m == static_cast<double>(SpeedMode::FixedAcc)) return current - value;`,
    },
    {
      note: "calc_v 的 Acc 模式漏掉 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::Acc)) return current + value * direction;`,
      to: `  if (m == static_cast<double>(SpeedMode::Acc)) return current + value;`,
    },
    {
      note: "calc_v 的 FixedLf2 用了 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::FixedLf2)) return soft_target(current, value);`,
      to: `  if (m == static_cast<double>(SpeedMode::FixedLf2)) return soft_target(current, value * direction);`,
    },
    {
      note: "calc_v 的 AccTo 漏掉 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    const double scaled_acc = acc * direction;`,
      to: `    const double scaled_acc = acc;`,
    },
    {
      note: "calc_v 的 AccTo 目标漏掉 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    const double target = value * direction;
    const double scaled_acc = acc * direction;`,
      to: `    const double target = value;
    const double scaled_acc = acc * direction;`,
    },
    {
      note: "calc_v 的 AccTo 命中判定用严格大于",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    if (current >= target && scaled_acc > 0) return current;`,
      to: `    if (current > target && scaled_acc > 0) return current;`,
    },
    {
      note: "calc_v 的 AccTo 反向命中判定用严格小于",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    if (current <= target && scaled_acc < 0) return current;`,
      to: `    if (current < target && scaled_acc < 0) return current;`,
    },
    {
      note: "calc_v 的 AccTo 用减法推进",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    return current + scaled_acc;
  }`,
      to: `    return current - scaled_acc;
  }`,
    },
    {
      note: "calc_v 的 FixedAccTo 目标乘了 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (m == static_cast<double>(SpeedMode::FixedAccTo)) {
    const double target = value;`,
      to: `  if (m == static_cast<double>(SpeedMode::FixedAccTo)) {
    const double target = value * direction;`,
    },
    {
      note: "calc_v 的 FixedAccTo 加速度乘了 direction",
      file: "native/lfw/entity/calc_v.cpp",
      from: `    return current + acc;
  }
  return soft_target(current, value * direction);`,
      to: `    return current + acc * direction;
  }
  return soft_target(current, value * direction);`,
    },
    {
      note: "calc_v 的 falsy 判定被取消",
      file: "native/lfw/entity/calc_v.cpp",
      from: `bool falsy_num(double n) { return n == 0.0 || std::isnan(n); }`,
      to: `bool falsy_num(double n) { return false; }`,
    },
    {
      note: "calc_v 的 acc 默认值写成 1",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  const double acc = param_or(acc_value, 0.0);`,
      to: `  const double acc = param_or(acc_value, 1.0);`,
    },
    {
      note: "calc_v 的 direction 默认值写成 0",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  const double direction = param_or(direction_value, 1.0);`,
      to: `  const double direction = param_or(direction_value, 0.0);`,
    },
    {
      note: "calc_v 的非数字 mode 回退到 Acc",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  const double m = raw != nullptr ? *raw : -1.0;`,
      to: `  const double m = raw != nullptr ? *raw : 1.0;`,
    },
    {
      note: "soft_target 的第二个守卫被取消",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (current < target && target > 0) return target;
  if (current > target && target < 0) return target;`,
      to: `  if (current < target && target > 0) return target;
  if (current > target) return target;`,
    },
    {
      note: "soft_target 的第一个守卫被取消",
      file: "native/lfw/entity/calc_v.cpp",
      from: `  if (current < target && target > 0) return target;`,
      to: `  if (current < target) return target;`,
    },
    {
      note: "same_face 的比较取反",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  return strict_equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? 1.0 : -1.0;`,
      to: `  return strict_equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? -1.0 : 1.0;`,
    },
    {
      note: "same_face 用松相等",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  return strict_equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? 1.0 : -1.0;`,
      to: `  return equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? 1.0 : -1.0;`,
    },
    {
      note: "same_face 只读 ref 的 facing",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  return strict_equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? 1.0 : -1.0;`,
      to: `  return strict_equals(field_or(ref, u"facing"), field_or(ref, u"facing")) ? 1.0 : -1.0;`,
    },
    {
      note: "turn_face 的 undefined 判定写成 null",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  if (std::holds_alternative<std::monostate>(f)) return Value();`,
      to: `  if (std::holds_alternative<NullTag>(f)) return Value();`,
    },
    {
      note: "turn_face 的判定常数写成 -1",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  return strict_equals(f, Value(1.0)) ? Value(-1.0) : Value(1.0);`,
      to: `  return strict_equals(f, Value(-1.0)) ? Value(-1.0) : Value(1.0);`,
    },
    {
      note: "turn_face 用松相等",
      file: "native/lfw/entity/face_helper.cpp",
      from: `  return strict_equals(f, Value(1.0)) ? Value(-1.0) : Value(1.0);`,
      to: `  return equals(f, Value(1.0)) ? Value(-1.0) : Value(1.0);`,
    },
    {
      note: "find_direction 漏掉 pair 的 falsy 短路",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `  if (!truthy(pair)) return 0.0;
  const Value id = field_or(frame, u"id");`,
      to: `  const Value id = field_or(frame, u"id");`,
    },
    {
      note: "find_direction 读错 -1 槽",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `  const Value a = field_or(pair, u"-1");`,
      to: `  const Value a = field_or(pair, u"1");`,
    },
    {
      note: "find_direction 读错 1 槽",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `  const Value b = field_or(pair, u"1");`,
      to: `  const Value b = field_or(pair, u"-1");`,
    },
    {
      note: "find_direction 返回的方向互换",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `  if (equals(a, id) || array_contains(a, id)) return -1.0;
  if (equals(b, id) || array_contains(b, id)) return 1.0;`,
      to: `  if (equals(a, id) || array_contains(a, id)) return 1.0;
  if (equals(b, id) || array_contains(b, id)) return -1.0;`,
    },
    {
      note: "find_direction 漏掉数组分支",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `  if (equals(a, id) || array_contains(a, id)) return -1.0;`,
      to: `  if (equals(a, id)) return -1.0;`,
    },
    {
      note: "find_direction 的数组匹配用严相等",
      file: "native/lfw/entity/find_frame_direction.cpp",
      from: `    if (equals(a->at(i), needle)) return true;`,
      to: `    if (strict_equals(a->at(i), needle)) return true;`,
    },
    {
      note: "type_is 用松相等",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  return strict_equals(field_or(v, u"type"), Value(static_cast<double>(want)));`,
      to: `  return equals(field_or(v, u"type"), Value(static_cast<double>(want)));`,
    },
    {
      note: "is_entity_data 用错枚举",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `bool is_entity_data(const Value& v) { return type_is(v, EntityEnum::Entity); }`,
      to: `bool is_entity_data(const Value& v) { return type_is(v, EntityEnum::Fighter); }`,
    },
    {
      note: "is_ball_data 用错枚举",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `bool is_ball_data(const Value& v) { return type_is(v, EntityEnum::Ball); }`,
      to: `bool is_ball_data(const Value& v) { return type_is(v, EntityEnum::Weapon); }`,
    },
    {
      note: "is_fighter 读了错误的键",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `bool is_fighter(const Value& v) { return is_fighter_data(field_or(v, u"data")); }`,
      to: `bool is_fighter(const Value& v) { return is_fighter_data(field_or(v, u"Data")); }`,
    },
    {
      note: "is_object_data 漏掉 Ball 分支",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  return is_entity_data(v) || is_fighter_data(v) || is_weapon_data(v) || is_ball_data(v);`,
      to: `  return is_entity_data(v) || is_fighter_data(v) || is_weapon_data(v);`,
    },
    {
      note: "is_bg_data 用松相等",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  return strict_equals(field_or(v, u"type"), Value(std::u16string(u"background")));`,
      to: `  return equals(field_or(v, u"type"), Value(std::u16string(u"background")));`,
    },
    {
      note: "is_boss 漏掉 object_data 前置判定",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  if (!is_object_data(data)) return false;
  const Value group = field_or(field_or(data, u"base"), u"group");`,
      to: `  const Value group = field_or(field_or(data, u"base"), u"group");`,
    },
    {
      note: "is_boss 的 group 空判定写成恰好 1 个",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  if (group_length(group) == 0) return false;`,
      to: `  if (group_length(group) == 1) return false;`,
    },
    {
      note: "is_boss 读错 base 键",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  const Value group = field_or(field_or(data, u"base"), u"group");`,
      to: `  const Value group = field_or(field_or(data, u"Base"), u"group");`,
    },
    {
      note: "is_boss 读错 group 键",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  const Value group = field_or(field_or(data, u"base"), u"group");`,
      to: `  const Value group = field_or(field_or(data, u"base"), u"Group");`,
    },
    {
      note: "is_boss 的组名比较用严相等",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `    if (equals(a->at(i), Value(std::u16string(entity_group::kBoss)))) return true;`,
      to: `    if (strict_equals(a->at(i), Value(std::u16string(entity_group::kBoss)))) return true;`,
    },
    {
      note: "is_boss 的组名常量写错",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `    if (equals(a->at(i), Value(std::u16string(entity_group::kBoss)))) return true;`,
      to: `    if (equals(a->at(i), Value(std::u16string(entity_group::kRegular)))) return true;`,
    },
    {
      note: "flag_is_true 只看键存在",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  return b != nullptr && *b;`,
      to: `  return b != nullptr;`,
    },
    {
      note: "flag_is_true 不做 bool 窄化",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `  const bool* b = std::get_if<bool>(&flag);
  return b != nullptr && *b;`,
      to: `  const bool* b = std::get_if<bool>(&flag);
  return b != nullptr ? *b : truthy(flag);`,
    },
    {
      note: "is_base_ctrl 读错标记键",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `bool is_base_ctrl(const Value& v) { return flag_is_true(v, u"__is_base_ctrl__"); }`,
      to: `bool is_base_ctrl(const Value& v) { return flag_is_true(v, u"__is_bot_ctrl__"); }`,
    },
    {
      note: "is_ball_ctrl 读错标记键",
      file: "native/lfw/entity/entity_type_check.cpp",
      from: `bool is_ball_ctrl(const Value& v) { return flag_is_true(v, u"__is_ball_ctrl__"); }`,
      to: `bool is_ball_ctrl(const Value& v) { return flag_is_true(v, u"__is_base_ctrl__"); }`,
    },
    {
      note: "NSlot 首项起始值写错（整表右移）",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"WAIT", static_cast<double>(NSlot::WAIT)},`,
      to: `      {u"WAIT", static_cast<double>(NSlot::WAIT) + 1},`,
    },
    {
      note: "NSlot 的 RESTING 取值写错",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"RESTING", static_cast<double>(NSlot::RESTING)},`,
      to: `      {u"RESTING", static_cast<double>(NSlot::RESTING_MAX)},`,
    },
    {
      note: "NSlot 的 HP 取值写错",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"HP", static_cast<double>(NSlot::HP)},`,
      to: `      {u"HP", static_cast<double>(NSlot::HP_MAX)},`,
    },
    {
      note: "NSlot 表漏掉一项",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"DROPPING", static_cast<double>(NSlot::DROPPING)},
      {u"COUNT", static_cast<double>(NSlot::COUNT)},`,
      to: `      {u"COUNT", static_cast<double>(NSlot::COUNT)},`,
    },
    {
      note: "SSlot 的 TEAM 取值写错",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"TEAM", static_cast<double>(SSlot::TEAM)},`,
      to: `      {u"TEAM", static_cast<double>(SSlot::NAME)},`,
    },
    {
      note: "SSlot 的 COPIES 取值写错",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `      {u"COPIES", static_cast<double>(SSlot::COPIES)},`,
      to: `      {u"COPIES", static_cast<double>(SSlot::DEAD_JOIN)},`,
    },
    {
      note: "NUM_SLOTS 写成 COUNT-1",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `double num_slots() { return static_cast<double>(NSlot::COUNT); }`,
      to: `double num_slots() { return static_cast<double>(NSlot::COUNT) - 1.0; }`,
    },
    {
      note: "STR_SLOTS 写成 0",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `double str_slots() { return static_cast<double>(SSlot::COUNT); }`,
      to: `double str_slots() { return static_cast<double>(SSlot::ID); }`,
    },
    {
      note: "num_or_null 的 NaN 判定写成 isinf",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  if (d != nullptr && std::isnan(*d)) return Value(NullTag{});`,
      to: `  if (d != nullptr && std::isinf(*d)) return Value(NullTag{});`,
    },
    {
      note: "num_or_null 总是返回 null",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  const double* d = std::get_if<double>(&v);
  if (d != nullptr && std::isnan(*d)) return Value(NullTag{});
  return v;`,
      to: `  return Value(NullTag{});`,
    },
    {
      note: "to_tri 漏掉空值分支",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) return -1.0;
  return truthy(v) ? 1.0 : 0.0;`,
      to: `  return truthy(v) ? 1.0 : 0.0;`,
    },
    {
      note: "to_tri 的空值编码写成 0",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) return -1.0;`,
      to: `  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) return 0.0;`,
    },
    {
      note: "to_tri 的真值编码互换",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  return truthy(v) ? 1.0 : 0.0;`,
      to: `  return truthy(v) ? 0.0 : 1.0;`,
    },
    {
      note: "from_tri 的负值判定用闭区间",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  if (lt(v, Value(0.0))) return Value(NullTag{});`,
      to: `  if (le(v, Value(0.0))) return Value(NullTag{});`,
    },
    {
      note: "from_tri 的零判定用松相等",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  return Value(!strict_equals(v, Value(0.0)));`,
      to: `  return Value(!equals(v, Value(0.0)));`,
    },
    {
      note: "from_tri 的布尔取反",
      file: "native/lfw/entity/entity_snapshot.cpp",
      from: `  return Value(!strict_equals(v, Value(0.0)));`,
      to: `  return Value(strict_equals(v, Value(0.0)));`,
    },
  ],
};
