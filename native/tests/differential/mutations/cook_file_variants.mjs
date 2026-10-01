export default {
  subject: "cook_file_variants",
  mutations: [
    {
      note: "偶数判定反转",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `  if (file_keys.empty() || file_keys.size() % 2 != 0) return;`,
      to: `  if (file_keys.empty() || file_keys.size() % 2 == 0) return;`,
    },
    {
      note: "不按字典序排序（改成反转）",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `  std::sort(file_keys.begin(), file_keys.end());`,
      to: `  std::reverse(file_keys.begin(), file_keys.end());`,
    },
    {
      note: "取第一个点而不是最后一个点",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == u'.') last_dot = i;
  }`,
      to: `  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == u'.') {
      last_dot = i;
      break;
    }
  }`,
    },
    {
      note: "字母表起点写成 c",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `char16_t letter_of(int offset) { return static_cast<char16_t>(98 + offset); }`,
      to: `char16_t letter_of(int offset) { return static_cast<char16_t>(99 + offset); }`,
    },
    {
      note: "gap 一致性判定反转",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `    gap_v = (to_number(gap_v) == diff) ? gap_v : Value(NullTag{});`,
      to: `    gap_v = (to_number(gap_v) != diff) ? gap_v : Value(NullTag{});`,
    },
    {
      note: "gap 不一致时写成 1（继续处理）",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `    gap_v = (to_number(gap_v) == diff) ? gap_v : Value(NullTag{});`,
      to: `    gap_v = (to_number(gap_v) == diff) ? gap_v : n(1);`,
    },
    {
      note: "不再比较 col",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `      if (!path_ok || !same_field(*variant, *tmpl, u"col") ||
          !same_field(*variant, *tmpl, u"row") || !same_field(*variant, *tmpl, u"cell_w") ||
          !same_field(*variant, *tmpl, u"cell_h")) {`,
      to: `      if (!path_ok || !same_field(*variant, *tmpl, u"row") ||
          !same_field(*variant, *tmpl, u"cell_w") || !same_field(*variant, *tmpl, u"cell_h")) {`,
    },
    {
      note: "不再比较 cell_h",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `      if (!path_ok || !same_field(*variant, *tmpl, u"col") ||
          !same_field(*variant, *tmpl, u"row") || !same_field(*variant, *tmpl, u"cell_w") ||
          !same_field(*variant, *tmpl, u"cell_h")) {`,
      to: `      if (!path_ok || !same_field(*variant, *tmpl, u"col") ||
          !same_field(*variant, *tmpl, u"row") || !same_field(*variant, *tmpl, u"cell_w")) {`,
    },
    {
      note: "变体字母偏移多 1",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `      const bool path_ok =
          path_of(infos[j]) == path_of(infos[i]) + std::u16string(1, letter_of(static_cast<int>(idx) - 1));`,
      to: `      const bool path_ok =
          path_of(infos[j]) == path_of(infos[i]) + std::u16string(1, letter_of(static_cast<int>(idx)));`,
    },
    {
      note: "variants 取 path 而不是 id",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `      variants.push_back(src != nullptr ? field_or_any(*src, u"id") : Value());`,
      to: `      variants.push_back(src != nullptr ? field_or_any(*src, u"path") : Value());`,
    },
    {
      note: "variants 多含自身（从下标 0 开始）",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `    for (size_t idx = 1; idx < indexes.size(); ++idx) {
      const size_t j = static_cast<size_t>(indexes[idx]) + i;
      const Object* src = j < infos.size() ? as_object(infos[j]) : nullptr;`,
      to: `    for (size_t idx = 0; idx < indexes.size(); ++idx) {
      const size_t j = static_cast<size_t>(indexes[idx]) + i;
      const Object* src = j < infos.size() ? as_object(infos[j]) : nullptr;`,
    },
    {
      note: "基准信息取 infos[1]",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `  const std::u16string first_str = path_of(infos[0]);`,
      to: `  const std::u16string first_str = path_of(infos[1]);`,
    },
    {
      note: "找变体时字母多往后取一个",
      file: "native/lfw/dat_translator/cook_file_variants.cpp",
      from: `    const std::u16string want = first_str + std::u16string(1, letter_of(i));`,
      to: `    const std::u16string want = first_str + std::u16string(1, letter_of(i + 1));`,
    },
  ],
};
