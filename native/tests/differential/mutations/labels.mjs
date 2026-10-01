export default {
  subject: "labels",
  mutations: [
    {
      note: "bdy_kind_name 的 falsy 回退丢掉（改成只看存在）",
      file: "native/lfw/defines/labels.cpp",
      from: `  if (it != m.end() && truthy(it->second)) return it->second;`,
      to: `  if (it != m.end()) return it->second;`,
    },
    {
      note: "wpoint_kind_name 的 nullish 回退错用 truthy",
      file: "native/lfw/defines/labels.cpp",
      from: `  if (it != m.end() && !std::holds_alternative<std::monostate>(it->second) &&
      !std::holds_alternative<NullTag>(it->second)) {`,
      to: `  if (it != m.end() && truthy(it->second)) {`,
    },
    {
      note: "枚举表不建反向（数字→名）表项",
      file: "native/lfw/defines/labels.cpp",
      from: `    m[number_to_string(e.value)] = Value(std::u16string(e.name));`,
      to: ``,
    },
    {
      note: "枚举表不建正向（名→数字）表项",
      file: "native/lfw/defines/labels.cpp",
      from: `    m[std::u16string(e.name)] = Value(e.value);`,
      to: `    (void)e.value;`,
    },
    {
      note: "hit flag 记忆化用数值键而不是原始值键",
      file: "native/lfw/defines/labels.cpp",
      from: `  memo[to_string(v)] = ret;`,
      to: `  memo[num_key] = ret;`,
    },
    {
      note: "hit flag 记忆化不合并回表",
      file: "native/lfw/defines/labels.cpp",
      from: `  for (const std::pair<const std::u16string, std::u16string>& kv : hit_flag_memo()) {
    out[kv.first] = kv.second;
  }`,
      to: `  (void)hit_flag_memo();`,
    },
    {
      note: "hit flag 位运算改用 static_cast<int> 而不是 ToInt32",
      file: "native/lfw/defines/labels.cpp",
      from: `  const int32_t vi = js_to_int32(num);`,
      to: `  const int32_t vi = static_cast<int32_t>(num);`,
    },
    {
      note: "hit flag 组合名忘了加分隔符",
      file: "native/lfw/defines/labels.cpp",
      from: `    if (!joined.empty()) joined += u"|";`,
      to: ``,
    },
    {
      note: "hit flag 基础位漏掉 Dead",
      file: "native/lfw/defines/labels.cpp",
      from: `  static const double kBase[] = {1, 2, 4, 8, 16, 32, 128};`,
      to: `  static const double kBase[] = {1, 2, 4, 8, 16, 32};`,
    },
    {
      note: "hit flag 未知名的前缀文案改错",
      file: "native/lfw/defines/labels.cpp",
      from: `  const std::u16string ret = joined.empty() ? (u"unknown_" + to_string(v)) : joined;`,
      to: `  const std::u16string ret = joined.empty() ? (u"unknown" + to_string(v)) : joined;`,
    },
    {
      note: "hitdesc 不再查描述表",
      file: "native/lfw/defines/labels.cpp",
      from: `  const std::u16string* d = table_string(u"HIT_FLAG_DESC_MAP", to_string(v));`,
      to: `  const std::u16string* d = nullptr;`,
    },
    {
      note: "hitdesc 的查表键改用数值形式",
      file: "native/lfw/defines/labels.cpp",
      from: `  const std::u16string* d = table_string(u"HIT_FLAG_DESC_MAP", to_string(v));`,
      to: `  const std::u16string* d = table_string(u"HIT_FLAG_DESC_MAP", number_to_string(to_number(v)));`,
    },
    {
      note: "full name 前缀写错（AllyFlag 是原代码里的错字，必须原样保留）",
      file: "native/lfw/defines/labels.cpp",
      from: `  return u"AllyFlag." + get_hit_flag_name(v);`,
      to: `  return u"HitFlag." + get_hit_flag_name(v);`,
    },
    {
      note: "bdy full name 前缀写错",
      file: "native/lfw/defines/labels.cpp",
      from: `  return u"BdyKind." + to_string(bdy_kind_name(v));`,
      to: `  return u"Bdy." + to_string(bdy_kind_name(v));`,
    },
    {
      note: "wpoint full name 前缀写错",
      file: "native/lfw/defines/labels.cpp",
      from: `  return u"WpointKind." + to_string(wpoint_kind_name(v));`,
      to: `  return u"WPointKind." + to_string(wpoint_kind_name(v));`,
    },
  ],
};
