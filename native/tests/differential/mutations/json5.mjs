export default {
  subject: "json5",
  mutations: [
    {
      note: "default: '/' 不再进入注释状态",
      file: "native/lfw/core/json5.cpp",
      from: `            case u'/':
              read();
              lex_state = LexState::kComment;
              continue;
`,
      to: ``,
    },
    {
      note: "peek(): 不合并代理对",
      file: "native/lfw/core/json5.cpp",
      from: `    if (u >= 0xD800 && u <= 0xDBFF && pos + 1 < src.size()) {`,
      to: `    if (u >= 0xE000 && u <= 0xFFFF && pos + 1 < src.size()) {`,
    },
    {
      note: "read(): 代理对只前进 1 个 code unit",
      file: "native/lfw/core/json5.cpp",
      from: `    const size_t len = ch == kEof ? 0 : (ch > 0xFFFF ? 2 : 1);`,
      to: `    const size_t len = ch == kEof ? 0 : 1;`,
    },
    {
      note: "fail_identifier(): 丢掉 column -= 5",
      file: "native/lfw/core/json5.cpp",
      from: `    column -= 5;
    error = u"JSON5: invalid identifier character at "`,
      to: `    error = u"JSON5: invalid identifier character at "`,
    },
    {
      note: "escape(): \\0 后面不检查数字",
      file: "native/lfw/core/json5.cpp",
      from: `      case u'0':
        read();
        if (json5_is_digit(peek())) {
          fail_char(read());
          return std::u16string();
        }
        return std::u16string(1, 0);`,
      to: `      case u'0':
        read();
        return std::u16string(1, 0);`,
    },
    {
      note: "zero: 丢掉符号（-0 变 +0）",
      file: "native/lfw/core/json5.cpp",
      from: `          out.kind = TokKind::kNumeric;
          out.num = sign * 0;
          return true;`,
      to: `          out.kind = TokKind::kNumeric;
          out.num = 0;
          return true;`,
    },
    {
      note: "value: '+' 也被当成负号",
      file: "native/lfw/core/json5.cpp",
      from: `              if (read() == u'-') sign = -1;`,
      to: `              read();
              sign = -1;`,
    },
    {
      note: "string: 引号闭合判断反转",
      file: "native/lfw/core/json5.cpp",
      from: `          if (c == u'"') {
            if (double_quote) {`,
      to: `          if (c == u'"') {
            if (!double_quote) {`,
    },
    {
      note: "afterPropertyValue: 漏掉 '}'",
      file: "native/lfw/core/json5.cpp",
      from: `        case LexState::kAfterPropertyValue:
          if (c == u',' || c == u'}') {`,
      to: `        case LexState::kAfterPropertyValue:
          if (c == u',') {`,
    },
    {
      note: "hexEscape: 第二个十六进制位不校验",
      file: "native/lfw/core/json5.cpp",
      from: `    buf.push_back(static_cast<char16_t>(read()));
    if (!json5_is_hex_digit(peek())) {
      fail_char(read());
      return 0;
    }
    buf.push_back(static_cast<char16_t>(read()));`,
      to: `    buf.push_back(static_cast<char16_t>(read()));
    buf.push_back(static_cast<char16_t>(read()));`,
    },
    {
      note: "unicodeEscape: 少读一位",
      file: "native/lfw/core/json5.cpp",
      from: `    for (int k = 0; k < 4; ++k) {`,
      to: `    for (int k = 0; k < 3; ++k) {`,
    },
    {
      note: "identifierNameStartEscape: 不校验码点类别",
      file: "native/lfw/core/json5.cpp",
      from: `          if (u != u'$' && u != u'_' && !json5_is_id_start_char(u)) return fail_identifier();`,
      to: ``,
    },
    {
      note: "str: 引号选择反转",
      file: "native/lfw/core/json5.cpp",
      from: `    const char16_t quote = n_single < n_double ? u'\\'' : u'"';`,
      to: `    const char16_t quote = n_single < n_double ? u'"' : u'\\'';`,
    },
    {
      note: "str: 不转义选中的引号",
      file: "native/lfw/core/json5.cpp",
      from: `      if (c == quote) out.push_back(u'\\\\');
`,
      to: ``,
    },
    {
      note: "str: \\0 后跟数字不特判",
      file: "native/lfw/core/json5.cpp",
      from: `      if (c == 0 && i + 1 < value.size() && json5_is_digit(value[i + 1])) {`,
      to: `      if (false) {`,
    },
    {
      note: "str: 空格也被 \\x 化",
      file: "native/lfw/core/json5.cpp",
      from: `      if (c < u' ') {`,
      to: `      if (c < u'!') {`,
    },
    {
      note: "str: 键的后续字符用 id_start 判定",
      file: "native/lfw/core/json5.cpp",
      from: `      if (!json5_is_id_continue_char(cp_at(key, i))) return quote_string(key);`,
      to: `      if (!json5_is_id_start_char(cp_at(key, i))) return quote_string(key);`,
    },
    {
      note: "str: 键首字符长度恒为 1",
      file: "native/lfw/core/json5.cpp",
      from: `    const size_t first_len = first > 0xFFFF ? 2 : 1;`,
      to: `    const size_t first_len = 1;`,
    },
    {
      note: "str: cp_at 不合并代理对",
      file: "native/lfw/core/json5.cpp",
      from: `  if (u >= 0xD800 && u <= 0xDBFF && i + 1 < s.size()) {`,
      to: `  if (u >= 0xE000 && u <= 0xFFFF && i + 1 < s.size()) {`,
    },
    {
      note: "str: 对象成员漏逗号",
      file: "native/lfw/core/json5.cpp",
      from: `      if (!first_member) out.push_back(u',');
`,
      to: ``,
    },
    {
      note: "str: 冒号后多一个空格",
      file: "native/lfw/core/json5.cpp",
      from: `      out.push_back(u':');`,
      to: `      out += u": ";`,
    },
    {
      note: "str: 字符串值不加引号",
      file: "native/lfw/core/json5.cpp",
      from: `    if (const std::u16string* s = std::get_if<std::u16string>(&value)) return quote_string(*s);`,
      to: `    if (const std::u16string* s = std::get_if<std::u16string>(&value)) return *s;`,
    },
    {
      note: "str: 布尔值互换",
      file: "native/lfw/core/json5.cpp",
      from: `    if (const bool* b = std::get_if<bool>(&value)) return *b ? u"true" : u"false";`,
      to: `    if (const bool* b = std::get_if<bool>(&value)) return *b ? u"false" : u"true";`,
    },
    {
      note: "str: null 序列化成空串",
      file: "native/lfw/core/json5.cpp",
      from: `    if (std::holds_alternative<NullTag>(value)) return u"null";`,
      to: `    if (std::holds_alternative<NullTag>(value)) return u"";`,
    },
  ],
};
