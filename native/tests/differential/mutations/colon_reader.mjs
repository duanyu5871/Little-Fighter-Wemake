export default {
  subject: "colon_reader",
  mutations: [
    {
      note: "rem 改成「正确」的删除（丢掉原代码从头部等长删除的手误行为）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `    rem = rem.substr(0, m.start) + rem.substr(m.end - m.start);`,
      to: `    rem = rem.substr(0, m.start) + rem.substr(m.end);`,
    },
    {
      note: "int_2 不再回溯（第一段吃掉全部数字）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `    for (size_t k = d1; k >= 1; --k) {`,
      to: `    for (size_t k = d1; k >= d1; --k) {`,
    },
    {
      note: "int_2 的第二段只取一位数字",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `      const size_t d2 = skip_digits(text, q) - q;`,
      to: `      const size_t d2 = skip_digits(text, q) > q ? 1 : 0;`,
    },
    {
      note: "名字后完全不跳空白（宽度与冒号之间的空格不被允许）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `  size_t p = skip_spaces(text, pos + cell.name.size());`,
      to: `  size_t p = pos + cell.name.size();`,
    },
    {
      note: "冒号后的 \\s* 只跳一个空白",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `  p = skip_spaces(text, p + 1);`,
      to: `  p = p + 1;\n  if (p < text.size() && is_str_white_space(text[p])) ++p;`,
    },
    {
      note: "名字匹配变成前缀匹配（w 能匹配 width）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `  if (text.compare(pos, cell.name.size(), cell.name) != 0) return m;`,
      to: `  if (text.compare(pos, cell.name.size() - 1, cell.name.substr(0, cell.name.size() - 1)) != 0) return m;`,
    },
    {
      note: "尾随字符类丢掉 |",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `bool is_tail(char16_t c) { return is_str_white_space(c) || c == u'|'; }`,
      to: `bool is_tail(char16_t c) { return is_str_white_space(c); }`,
    },
    {
      note: "尾随字符类丢掉空白",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `bool is_tail(char16_t c) { return is_str_white_space(c) || c == u'|'; }`,
      to: `bool is_tail(char16_t c) { return c == u'|'; }`,
    },
    {
      note: "str 单元格允许吃空白（\\S+ 变成到行尾）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `    while (p < text.size() && !is_str_white_space(text[p])) ++p;`,
      to: `    p = text.size();`,
    },
    {
      note: "允许空捕获",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `    p = skip_digits(text, p);
    if (p == s) return m;
    m.c1 = text.substr(s, p - s);`,
      to: `    p = skip_digits(text, p);
    m.c1 = text.substr(s, p - s);`,
    },
    {
      note: "左匹配从下标 1 开始（漏掉行首）",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `  for (size_t pos = 0; pos < text.size(); ++pos) {`,
      to: `  for (size_t pos = 1; pos < text.size(); ++pos) {`,
    },
    {
      note: "某个单元格找不到就中断后续单元格",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `    const Match m = find_match(rem, cell);
    if (!m.ok) continue;`,
      to: `    const Match m = find_match(rem, cell);
    if (!m.ok) break;`,
    },
    {
      note: "int_2 的第二段值写成第一段",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `      a.push_back(Value(to_number(Value(m.c2))));`,
      to: `      a.push_back(Value(to_number(Value(m.c1))));`,
    },
    {
      note: "int 单元格存成字符串",
      file: "native/lfw/dat_translator/colon_value_reader.cpp",
      from: `      output.set(cell.name, Value(to_number(Value(m.c1))));`,
      to: `      output.set(cell.name, Value(m.c1));`,
    },
  ],
};
