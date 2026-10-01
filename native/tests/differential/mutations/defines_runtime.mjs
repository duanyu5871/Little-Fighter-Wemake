export default {
  subject: "defines_runtime",
  mutations: [
    {
      note: "生成的数据表：WT_BOUNCE_MIN_Y 的 Baseball 值改错",
      file: "native/lfw/defines/runtime_gen.cpp",
      from: `u"{\\"0\\":2,\\"1\\":2,\\"2\\":2,\\"3\\":2,\\"4\\":1,\\"5\\":1}"`,
      to: `u"{\\"0\\":2,\\"1\\":2,\\"2\\":2,\\"3\\":2,\\"4\\":2,\\"5\\":1}"`,
    },
    {
      note: "desire 少乘/多加（round(MAX_AI_DESIRE * ratio) 改错）",
      file: "native/lfw/defines/defines.cpp",
      from: `double desire(double ratio) { return round(num(u"Defines.MAX_AI_DESIRE") * ratio); }`,
      to: `double desire(double ratio) { return round(num(u"Defines.MAX_AI_DESIRE") * ratio) + 1; }`,
    },
    {
      note: "desire 读错常量名",
      file: "native/lfw/defines/defines.cpp",
      from: `return round(num(u"Defines.MAX_AI_DESIRE") * ratio);`,
      to: `return round(num(u"Defines.NO_SUCH_CONST") * ratio);`,
    },
    {
      note: "is_independent 的 !== 1 写成 > 1",
      file: "native/lfw/defines/defines.cpp",
      from: `bool is_independent(const std::u16string& team) { return team.size() != 1; }`,
      to: `bool is_independent(const std::u16string& team) { return team.size() > 1; }`,
    },
    {
      note: "is_cheat_type 少认一个成员",
      file: "native/lfw/defines/defines.cpp",
      from: `v == std::u16string(cheat_enum::kGIM_INK);`,
      to: `v == std::u16string(cheat_enum::kHERO_FT);`,
    },
    {
      note: "is_difficulty 边界成员写错",
      file: "native/lfw/defines/defines.cpp",
      from: `v == static_cast<double>(Difficulty::Difficult) || v == static_cast<double>(Difficulty::Crazy);`,
      to: `v == static_cast<double>(Difficulty::Difficult) || v == static_cast<double>(Difficulty::MIN);`,
    },
    {
      note: "get_default_keys 取错了 key",
      file: "native/lfw/defines/defines.cpp",
      from: `  const Value* exact = o->get(player_id);`,
      to: `  const Value* exact = o->get(u"1");`,
    },
    {
      note: "get_default_keys 的兜底 key 写错",
      file: "native/lfw/defines/defines.cpp",
      from: `  const Value* fallback = o->get(u"_");`,
      to: `  const Value* fallback = o->get(u"1");`,
    },
    {
      note: "table 查找比较取反",
      file: "native/lfw/defines/defines.cpp",
      from: `    if (kv.first == name) return &kv.second;`,
      to: `    if (kv.first != name) return &kv.second;`,
    },
  ],
};
