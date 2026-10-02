/**
 * json 变异规格
 *
 * 覆盖 subject: json（lfw/core/json.cpp 的 json_stringify / json_parse）
 *
 * 已排除、**无法构造**的变异（避免"看似覆盖其实是空转"）：
 *  - quote() 里 `(c >> 12) & 0xf` 一项：该分支只在 `c < 0x20` 时进入，`c >> 12` 恒为 0
 *    ⇒ 改不改都输出 '0'，代数上等价。
 *  - Parser::str() 的 `if (i + 4 > s.size())` 改成 `i + 3 > s.size()`：能通过该检查又"恰好四字符都是
 *    十六进制"的输入不存在（真这样时后面一定跟着收尾引号或越界 → 两版都 fail）。
 *  - Parser::digit() 的 `i < s.size()` 改成 `i <= s.size()`：`s[size()]` 是 '\0'，不是数字，结果相同。
 *  - write() 末尾 `if (o == nullptr) return false;`：走到那里时 variant 只可能是 Object ⇒ 不可达。
 *  - 若干会造成越界读/死循环的改法（arr()/obj() 里去掉 `++i`、str() 里去掉越界判断、
 *    递归写成 `write(v, out)`）一律不打 —— 它们不是"漂移"而是 UB/挂死，无法作为覆盖证据。
 */
const raw = String.raw;

export default {
  subject: "json",
  mutations: [
    // ---- quote(): 转义表 ----
    {
      note: "quote: 双引号的 case 标签写错",
      file: "native/lfw/core/json.cpp",
      from: raw`      case u'"':
        out += u"\\\"";
        break;`,
      to: raw`      case u'X':
        out += u"\"";
        break;`,
    },
    {
      note: "quote: 反斜杠的 case 标签写错",
      file: "native/lfw/core/json.cpp",
      from: raw`      case u'\\':
        out += u"\\\\";
        break;`,
      to: raw`      case u'Y':
        out += u"\\\\";
        break;`,
    },
    {
      note: "quote: \\b 的 case 标签写成 0x07",
      file: "native/lfw/core/json.cpp",
      from: "      case 0x0008:",
      to: "      case 0x0007:",
    },
    {
      note: "quote: \\f 的 case 标签写成 0x0e",
      file: "native/lfw/core/json.cpp",
      from: "      case 0x000c:",
      to: "      case 0x000e:",
    },
    {
      note: "quote: \\n 的 case 标签写成 0x0b",
      file: "native/lfw/core/json.cpp",
      from: "      case 0x000a:",
      to: "      case 0x000b:",
    },
    {
      note: "quote: \\r 的 case 标签写成 0x0f",
      file: "native/lfw/core/json.cpp",
      from: "      case 0x000d:",
      to: "      case 0x000f:",
    },
    {
      note: "quote: \\t 的 case 标签写错",
      file: "native/lfw/core/json.cpp",
      from: "      case 0x0009:",
      to: "      case 0x0099:",
    },
    {
      note: "quote: 控制字符阈值写成 0x10",
      file: "native/lfw/core/json.cpp",
      from: "        if (c < 0x20) {",
      to: "        if (c < 0x10) {",
    },
    {
      note: "quote: 控制字符阈值含空格",
      file: "native/lfw/core/json.cpp",
      from: "        if (c < 0x20) {",
      to: "        if (c <= 0x20) {",
    },
    {
      note: "quote: 第三个十六进制位取错移位数",
      file: "native/lfw/core/json.cpp",
      from: "          out.push_back(kHexDigits[(c >> 4) & 0xf]);",
      to: "          out.push_back(kHexDigits[(c >> 8) & 0xf]);",
    },
    {
      note: "quote: 第四个十六进制位取错移位数",
      file: "native/lfw/core/json.cpp",
      from: "          out.push_back(kHexDigits[c & 0xf]);",
      to: "          out.push_back(kHexDigits[(c >> 4) & 0xf]);",
    },
    {
      note: "quote: 十六进制表写大写",
      file: "native/lfw/core/json.cpp",
      from: 'const char16_t* const kHexDigits = u"0123456789abcdef";',
      to: 'const char16_t* const kHexDigits = u"0123456789ABCDEF";',
    },
    {
      note: "quote: 忘了写起始引号",
      file: "native/lfw/core/json.cpp",
      from: "void quote(const std::u16string& s, std::u16string& out) {\n  out.push_back(u'\"');",
      to: "void quote(const std::u16string& s, std::u16string& out) {",
    },
    {
      note: "quote: 忘了写收尾引号",
      file: "native/lfw/core/json.cpp",
      from: "  out.push_back(u'\"');\n}",
      to: "}",
    },
    {
      note: "quote: \\u 前缀写成 \\x",
      file: "native/lfw/core/json.cpp",
      from: raw`          out += u"\\u";`,
      to: raw`          out += u"\\x";`,
    },

    // ---- write() ----
    {
      note: "write: 顶层 undefined 也当成功",
      file: "native/lfw/core/json.cpp",
      from: "  if (std::holds_alternative<std::monostate>(v)) return false;",
      to: "  if (std::holds_alternative<std::monostate>(v)) return true;",
    },
    {
      note: "write: null 拼错",
      file: "native/lfw/core/json.cpp",
      from: '  if (std::holds_alternative<NullTag>(v)) {\n    out += u"null";\n    return true;\n  }',
      to: '  if (std::holds_alternative<NullTag>(v)) {\n    out += u"nil";\n    return true;\n  }',
    },
    {
      note: "write: 布尔取反",
      file: "native/lfw/core/json.cpp",
      from: '    out += *b ? u"true" : u"false";',
      to: '    out += !*b ? u"true" : u"false";',
    },
    {
      note: "write: 有限性判断取反",
      file: "native/lfw/core/json.cpp",
      from:
        '    out += std::isfinite(*d) ? number_to_string(*d) : std::u16string(u"null");',
      to: '    out += !std::isfinite(*d) ? number_to_string(*d) : std::u16string(u"null");',
    },
    {
      note: "write: 非有限数写成 NaN 而不是 null",
      file: "native/lfw/core/json.cpp",
      from:
        '    out += std::isfinite(*d) ? number_to_string(*d) : std::u16string(u"null");',
      to: '    out += std::isfinite(*d) ? number_to_string(*d) : std::u16string(u"NaN");',
    },
    {
      note: "write: 字符串不加引号",
      file: "native/lfw/core/json.cpp",
      from:
        '  if (const std::u16string* s = std::get_if<std::u16string>(&v)) {\n    quote(*s, out);',
      to: '  if (const std::u16string* s = std::get_if<std::u16string>(&v)) {\n    out += *s;',
    },
    {
      note: "write: 数组里的 undefined 不写 null",
      file: "native/lfw/core/json.cpp",
      from:
        '      const Value& item = a->at(i);\n      if (std::holds_alternative<std::monostate>(item)) {\n        out += u"null";\n        continue;\n      }\n      write(item, out);',
      to: "      const Value& item = a->at(i);\n      write(item, out);",
    },
    {
      note: "write: 数组里的 undefined 写成别的字面量",
      file: "native/lfw/core/json.cpp",
      from:
        '      if (std::holds_alternative<std::monostate>(item)) {\n        out += u"null";\n        continue;\n      }',
      to:
        '      if (std::holds_alternative<std::monostate>(item)) {\n        out += u"undefined";\n        continue;\n      }',
    },
    {
      note: "write: 数组分隔符条件写错",
      file: "native/lfw/core/json.cpp",
      from: "      if (i != 0) out.push_back(u',');",
      to: "      if (i > 1) out.push_back(u',');",
    },
    {
      note: "write: 数组开括号写错",
      file: "native/lfw/core/json.cpp",
      from: "    out.push_back(u'[');",
      to: "    out.push_back(u'{');",
    },
    {
      note: "write: 数组闭括号写错",
      file: "native/lfw/core/json.cpp",
      from: "    out.push_back(u']');",
      to: "    out.push_back(u'}');",
    },
    {
      note: "write: 对象里 undefined 值不跳过",
      file: "native/lfw/core/json.cpp",
      from:
        "    if (p == nullptr || std::holds_alternative<std::monostate>(*p)) continue;",
      to: "    if (p == nullptr) continue;",
    },
    {
      note: "write: first 不复位",
      file: "native/lfw/core/json.cpp",
      from: "    first = false;",
      to: "    first = true;",
    },
    {
      note: "write: 对象分隔符条件写反",
      file: "native/lfw/core/json.cpp",
      from: "    if (!first) out.push_back(u',');",
      to: "    if (first) out.push_back(u',');",
    },
    {
      note: "write: 对象键不加引号",
      file: "native/lfw/core/json.cpp",
      from: "    quote(k, out);\n    out.push_back(u':');",
      to: "    out += k;\n    out.push_back(u':');",
    },
    {
      note: "write: 冒号写成等号",
      file: "native/lfw/core/json.cpp",
      from: "    out.push_back(u':');",
      to: "    out.push_back(u'=');",
    },
    {
      note: "write: 对象开括号写错",
      file: "native/lfw/core/json.cpp",
      from: "  out.push_back(u'{');",
      to: "  out.push_back(u'[');",
    },
    {
      note: "write: 对象闭括号写错",
      file: "native/lfw/core/json.cpp",
      from: "  out.push_back(u'}');",
      to: "  out.push_back(u']');",
    },
    {
      note: "json_stringify: 失败也返回文本",
      file: "native/lfw/core/json.cpp",
      from:
        "  std::u16string out;\n  if (!write(v, out)) return std::nullopt;\n  return out;",
      to: "  std::u16string out;\n  write(v, out);\n  return out;",
    },

    // ---- is_json_ws ----
    {
      note: "is_json_ws: 少了 tab",
      file: "native/lfw/core/json.cpp",
      from: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r';",
      to: "  return c == u' ' || c == u'\\n' || c == u'\\r';",
    },
    {
      note: "is_json_ws: 少了 LF",
      file: "native/lfw/core/json.cpp",
      from: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r';",
      to: "  return c == u' ' || c == u'\\t' || c == u'\\r';",
    },
    {
      note: "is_json_ws: 少了 CR",
      file: "native/lfw/core/json.cpp",
      from: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r';",
      to: "  return c == u' ' || c == u'\\t' || c == u'\\n';",
    },
    {
      note: "is_json_ws: 多认了换页符",
      file: "native/lfw/core/json.cpp",
      from: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r';",
      to: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r' || c == 0x000c;",
    },
    {
      note: "is_json_ws: 多认了不换行空格",
      file: "native/lfw/core/json.cpp",
      from: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r';",
      to: "  return c == u' ' || c == u'\\t' || c == u'\\n' || c == u'\\r' || c == 0x00a0;",
    },

    // ---- Parser::str ----
    {
      note: "str: 反斜杠判断写反",
      file: "native/lfw/core/json.cpp",
      from: raw`      if (c != u'\\') {
        out.push_back(c);
        continue;
      }`,
      to: raw`      if (c == u'\\') {
        out.push_back(c);
        continue;
      }`,
    },
    {
      note: "str: \\b 解成 0x0b",
      file: "native/lfw/core/json.cpp",
      from: "        case u'b':\n          out.push_back(0x0008);",
      to: "        case u'b':\n          out.push_back(0x000b);",
    },
    {
      note: "str: \\f 解成 0x0a",
      file: "native/lfw/core/json.cpp",
      from: "        case u'f':\n          out.push_back(0x000c);",
      to: "        case u'f':\n          out.push_back(0x000a);",
    },
    {
      note: "str: \\n 解成 0x0d",
      file: "native/lfw/core/json.cpp",
      from: "        case u'n':\n          out.push_back(0x000a);",
      to: "        case u'n':\n          out.push_back(0x000d);",
    },
    {
      note: "str: \\r 解成 0x0a",
      file: "native/lfw/core/json.cpp",
      from: "        case u'r':\n          out.push_back(0x000d);",
      to: "        case u'r':\n          out.push_back(0x000a);",
    },
    {
      note: "str: \\t 解成空格",
      file: "native/lfw/core/json.cpp",
      from: "        case u't':\n          out.push_back(0x0009);",
      to: "        case u't':\n          out.push_back(0x0020);",
    },
    {
      note: "str: 不给 \\/ 放行",
      file: "native/lfw/core/json.cpp",
      from: "        case u'/':\n          out.push_back(u'/');\n          break;",
      to: "        case u'/':\n          fail();\n          return out;",
    },
    {
      note: "str: 不认大写十六进制",
      file: "native/lfw/core/json.cpp",
      from:
        "            if (h >= u'0' && h <= u'9') d = static_cast<unsigned>(h - u'0');\n            else if (h >= u'a' && h <= u'f') d = static_cast<unsigned>(h - u'a') + 10;\n            else if (h >= u'A' && h <= u'F') d = static_cast<unsigned>(h - u'A') + 10;",
      to:
        "            if (h >= u'0' && h <= u'9') d = static_cast<unsigned>(h - u'0');\n            else if (h >= u'a' && h <= u'f') d = static_cast<unsigned>(h - u'a') + 10;",
    },
    {
      note: "str: 小写十六进制值偏移写错",
      file: "native/lfw/core/json.cpp",
      from: "d = static_cast<unsigned>(h - u'a') + 10;",
      to: "d = static_cast<unsigned>(h - u'a') + 9;",
    },
    {
      note: "str: 十六进制累积用十进制",
      file: "native/lfw/core/json.cpp",
      from: "            v = v * 16 + d;",
      to: "            v = v * 10 + d;",
    },
    {
      note: "str: \\uXXXX 只前进 3 个字符",
      file: "native/lfw/core/json.cpp",
      from: "          i += 4;\n          out.push_back(static_cast<char16_t>(v));",
      to: "          i += 3;\n          out.push_back(static_cast<char16_t>(v));",
    },
    {
      note: "str: 没有收尾引号也算成功",
      file: "native/lfw/core/json.cpp",
      from: "    }\n    fail();\n    return out;\n  }",
      to: "    }\n    return out;\n  }",
    },

    // ---- Parser::num ----
    {
      note: "num: 前导 0 后继续吃数字",
      file: "native/lfw/core/json.cpp",
      from: "    if (s[i] == u'0') {\n      ++i;\n    } else {",
      to: "    if (s[i] == u'0') {\n      while (digit()) ++i;\n    } else {",
    },
    {
      note: "num: 前导 0 的判断写成 9",
      file: "native/lfw/core/json.cpp",
      from: "    if (s[i] == u'0') {",
      to: "    if (s[i] == u'9') {",
    },
    {
      note: "num: 负号不识别",
      file: "native/lfw/core/json.cpp",
      from: "    if (i < s.size() && s[i] == u'-') ++i;",
      to: "    if (i < s.size() && s[i] == u'+') ++i;",
    },
    {
      note: "num: 小数点判断取反",
      file: "native/lfw/core/json.cpp",
      from: "    if (i < s.size() && s[i] == u'.') {",
      to: "    if (i < s.size() && s[i] != u'.') {",
    },
    {
      note: "num: 不认大写 E",
      file: "native/lfw/core/json.cpp",
      from: "    if (i < s.size() && (s[i] == u'e' || s[i] == u'E')) {",
      to: "    if (i < s.size() && s[i] == u'e') {",
    },
    {
      note: "num: 不认指数正号",
      file: "native/lfw/core/json.cpp",
      from: "      if (i < s.size() && (s[i] == u'+' || s[i] == u'-')) ++i;",
      to: "      if (i < s.size() && s[i] == u'-') ++i;",
    },
    {
      note: "num: 数字切片吃到末尾",
      file: "native/lfw/core/json.cpp",
      from: "    return Value(string_to_number(s.substr(start, i - start)));",
      to: "    return Value(string_to_number(s.substr(start)));",
    },

    // ---- Parser::arr ----
    {
      note: "arr: 空数组不接受",
      file: "native/lfw/core/json.cpp",
      from: "    if (i < s.size() && s[i] == u']') {\n      ++i;\n      return Value(a);\n    }",
      to: "    if (i < s.size() && s[i] == u']') {\n      return fail();\n    }",
    },
    {
      note: "arr: 元素后不吃空白",
      file: "native/lfw/core/json.cpp",
      from: "      a->push_back(val());\n      if (!ok) return Value();\n      ws();",
      to: "      a->push_back(val());\n      if (!ok) return Value();",
    },
    {
      note: "arr: 逗号后直接失败",
      file: "native/lfw/core/json.cpp",
      from: "      if (i < s.size() && s[i] == u',') {\n        ++i;\n        continue;\n      }\n      if (i < s.size() && s[i] == u']') {",
      to: "      if (i < s.size() && s[i] == u',') {\n        ++i;\n        return fail();\n      }\n      if (i < s.size() && s[i] == u']') {",
    },

    // ---- Parser::obj ----
    {
      note: "obj: 空对象不接受",
      file: "native/lfw/core/json.cpp",
      from: "    if (i < s.size() && s[i] == u'}') {\n      ++i;\n      return Value(o);\n    }",
      to: "    if (i < s.size() && s[i] == u'}') {\n      return fail();\n    }",
    },
    {
      note: "obj: 键必须是字符串的判断写反",
      file: "native/lfw/core/json.cpp",
      from: '      if (i >= s.size() || s[i] != u\'"\') return fail();',
      to: '      if (i >= s.size() || s[i] == u\'"\') return fail();',
    },
    {
      note: "obj: 不检查冒号",
      file: "native/lfw/core/json.cpp",
      from:
        "      ws();\n      if (i >= s.size() || s[i] != u':') return fail();\n      ++i;",
      to: "      ws();\n      ++i;",
    },
    {
      note: "obj: 重复键保留先到的",
      file: "native/lfw/core/json.cpp",
      from: "      o->set(k, val());",
      to: "      if (o->get(k) == nullptr) o->set(k, val());",
    },
    {
      note: "obj: 逗号后直接失败",
      file: "native/lfw/core/json.cpp",
      from: "      if (i < s.size() && s[i] == u',') {\n        ++i;\n        continue;\n      }\n      if (i < s.size() && s[i] == u'}') {",
      to: "      if (i < s.size() && s[i] == u',') {\n        ++i;\n        return fail();\n      }\n      if (i < s.size() && s[i] == u'}') {",
    },

    // ---- Parser::val ----
    {
      note: "val: true 只前进 3",
      file: "native/lfw/core/json.cpp",
      from: "      i += 4;\n      return Value(true);",
      to: "      i += 3;\n      return Value(true);",
    },
    {
      note: "val: false 只前进 4",
      file: "native/lfw/core/json.cpp",
      from: "      i += 5;\n      return Value(false);",
      to: "      i += 4;\n      return Value(false);",
    },
    {
      note: "val: null 前进 5",
      file: "native/lfw/core/json.cpp",
      from: "      i += 4;\n      return Value(NullTag{});",
      to: "      i += 5;\n      return Value(NullTag{});",
    },
    {
      note: "val: null 解成 undefined",
      file: "native/lfw/core/json.cpp",
      from: "      if (!at(u\"null\", 4)) return fail();\n      i += 4;\n      return Value(NullTag{});",
      to: "      if (!at(u\"null\", 4)) return fail();\n      i += 4;\n      return Value();",
    },
    {
      note: "val: 对象与数组入口互换",
      file: "native/lfw/core/json.cpp",
      from: "    if (c == u'{') return obj();\n    if (c == u'[') return arr();",
      to: "    if (c == u'{') return arr();\n    if (c == u'[') return obj();",
    },

    // ---- Parser::digit / json_parse ----
    {
      note: "digit: 不认 9",
      file: "native/lfw/core/json.cpp",
      from: "  bool digit() { return i < s.size() && s[i] >= u'0' && s[i] <= u'9'; }",
      to: "  bool digit() { return i < s.size() && s[i] >= u'0' && s[i] <= u'8'; }",
    },
    {
      note: "digit: 不认 0",
      file: "native/lfw/core/json.cpp",
      from: "  bool digit() { return i < s.size() && s[i] >= u'0' && s[i] <= u'9'; }",
      to: "  bool digit() { return i < s.size() && s[i] >= u'1' && s[i] <= u'9'; }",
    },
    {
      note: "json_parse: 不剥开头的空白",
      file: "native/lfw/core/json.cpp",
      from: "  Parser p{text};\n  p.ws();\n  Value v = p.val();",
      to: "  Parser p{text};\n  Value v = p.val();",
    },
    {
      note: "json_parse: 不剥结尾的空白",
      file: "native/lfw/core/json.cpp",
      from:
        "  if (!p.ok) return std::nullopt;\n  p.ws();\n  if (p.i != text.size()) return std::nullopt;",
      to: "  if (!p.ok) return std::nullopt;\n  if (p.i != text.size()) return std::nullopt;",
    },
    {
      note: "json_parse: 不检查尾部残留",
      file: "native/lfw/core/json.cpp",
      from: "  if (p.i != text.size()) return std::nullopt;",
      to: "  if (p.i > text.size()) return std::nullopt;",
    },
  ],
};
