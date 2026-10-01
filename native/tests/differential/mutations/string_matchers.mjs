export default {
  subject: "string_matchers",
  mutations: [
    {
      note: "match_colon_value: 值前的 \\s* 被漏掉（\"a: 1\" 的值变空）",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `      size_t v_start = j + 1;
      while (v_start < t.size() && is_ws(t[v_start])) ++v_start;`,
      to: `      size_t v_start = j + 1;`,
    },
    {
      note: "match_colon_value: 键不再回溯（\"a:b:c\" 的键变成 \"a:b\" 之前）",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    if (klen == 0) break;
    --klen;`,
      to: `    break;`,
    },
    {
      note: "match_colon_value: 匹配区间收在冒号之后（下一轮从那里继续）",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `      end_pos = v_end;`,
      to: `      end_pos = j + 1;`,
    },
    {
      note: "match_colon_value: 空白集合只认空格与制表符",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `bool is_ws(char16_t c) { return is_str_white_space(c); }`,
      to: `bool is_ws(char16_t c) { return c == u' ' || c == u'\\t'; }`,
    },
    {
      note: "match_colon_value: 分隔符写成 '='",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    if (j < t.size() && t[j] == u':') {`,
      to: `    if (j < t.size() && t[j] == u'=') {`,
    },
    {
      note: "block matcher: 允许空 body（<a></a> 也会命中）",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    if (body_from < t.size()) {
      const size_t e = t.find(end, body_from + 1);`,
      to: `    if (body_from <= t.size()) {
      const size_t e = t.find(end, body_from);`,
    },
    {
      note: "block matcher: start/end 不再 trim",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `  return find_block(t, trimmed(start), trimmed(end), from, m_start, m_end, body_start, body_end);`,
      to: `  return find_block(t, start, end, from, m_start, m_end, body_start, body_end);`,
    },
    {
      note: "take_blocks: 下一段起点多跳一个字符（相邻块会漏）",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    from = m_end;`,
      to: `    from = m_end + 1;`,
    },
    {
      note: "take_blocks: block 取的是整段匹配而非捕获组",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    ret.blocks.push_back(text.substr(body_start, body_end - body_start));`,
      to: `    ret.blocks.push_back(text.substr(m_start, m_end - m_start));`,
    },
    {
      note: "take_blocks: remains 挖掉的区间用反",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `    remains += text.substr(prev, p.first - prev);
    prev = p.second;`,
      to: `    remains += text.substr(prev, p.second - prev);
    prev = p.first;`,
    },
    {
      note: "take_blocks: 丢掉尾部残余",
      file: "native/lfw/dat_translator/string_matchers.cpp",
      from: `  remains += text.substr(prev);
  ret.remains = remains;`,
      to: `  ret.remains = remains;`,
    },
    {
      note: "delete_undefined: 判据写成 null",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `    if (v != nullptr && std::string(type_of(*v)) == "undefined") p->remove(k);`,
      to: `    if (v != nullptr && std::string(type_of(*v)) == "null") p->remove(k);`,
    },
    {
      note: "delete_undefined: 无差别删除所有键",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `    if (v != nullptr && std::string(type_of(*v)) == "undefined") p->remove(k);`,
      to: `    if (v != nullptr) p->remove(k);`,
    },
  ],
};
