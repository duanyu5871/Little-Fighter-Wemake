export default {
  subject: "cond_maker",
  mutations: [
    {
      note: "add 的允许集合漏掉 &&",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  opok(u"add", kAllowNone | kAllowOr | kAllowAnd);`,
      to: `  opok(u"add", kAllowNone | kAllowOr);`,
    },
    {
      note: "nullish 检查只看 undefined，漏掉 null",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<std::monostate>(v);`,
    },
    {
      note: "assert_value 不再接受 boolean",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (is_string(v) || is_number(v) || is_bool(v)) return true;`,
      to: `  if (is_string(v) || is_number(v)) return true;`,
    },
    {
      note: "assert_op 不再拦保留运算符",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (op == u"||" || op == u"&&" || op == u"!") {`,
      to: `  if (false) {`,
    },
    {
      note: "assert_op 不再拦空运算符",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (op.empty()) {`,
      to: `  if (false) {`,
    },
    {
      note: "or_ 在空 maker 上也插入分隔符",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `CondMaker& CondMaker::or_(const Value& v1, const std::u16string& op, const Value& v2) {
  if (!_error.empty()) return *this;
  if (!_parts.empty()) push_text(u"||");`,
      to: `CondMaker& CondMaker::or_(const Value& v1, const std::u16string& op, const Value& v2) {
  if (!_error.empty()) return *this;
  push_text(u"||");`,
    },
    {
      note: "done 丢掉「单个子构造器不加括号」的捷径",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (_parts.size() == 1 && _parts[0].is_maker) return _parts[0].maker->done();`,
      to: `  if (false) return u"";`,
    },
    {
      note: "done 不再对每个片段做 trim",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `      ret += js_trim(p.text);`,
      to: `      ret += p.text;`,
    },
    {
      note: "done 不再删除换行/回车",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `    if (c != u'\\n' && c != u'\\r') out.push_back(c);`,
      to: `    out.push_back(c);`,
    },
    {
      // 这两条原锚在 `cond_maker.cpp` 的本地空白判定上；该判定后来被抽成
      // `lfw::is_str_white_space`（`native/lfw/core/js_string.cpp`），故改锚到此。
      note: "js_trim 只按 ASCII 空格裁剪（U+3000 / U+FEFF 不再算空白）",
      file: "native/lfw/core/js_string.cpp",
      from: `    case 0x3000:
    case 0xfeff:
      return true;`,
      to: `      return true;`,
    },
    {
      note: "js_trim 忽略 NBSP",
      file: "native/lfw/core/js_string.cpp",
      from: `    case 0x0009:
    case 0x000a:
    case 0x000b:
    case 0x000c:
    case 0x000d:
    case 0x0020:
    case 0x00a0:
    case 0x1680:
    case 0x2028:
    case 0x2029:
    case 0x202f:
    case 0x205f:
    case 0x3000:
    case 0xfeff:
      return true;`,
      to: `    case 0x0009:
    case 0x000a:
    case 0x000b:
    case 0x000c:
    case 0x000d:
    case 0x0020:
    case 0x1680:
    case 0x2028:
    case 0x2029:
    case 0x3000:
    case 0xfeff:
      return true;`,
    },
    {
      note: "加引号时不再转义引号字符",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `    if (c == _quote_char) out.push_back(u'\\\\');`,
      to: `    if (false) out.push_back(u'\\\\');`,
    },
    {
      note: "子构造器的错误不再向上传播",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (!child->ok()) _error = child->error();`,
      to: `  (void)child;`,
    },
    {
      note: "子构造器不再继承引号设置",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  c->_quote_on = _quote_on;
  c->_quote_char = _quote_char;`,
      to: ``,
    },
    {
      note: "one_of 缺值时不报错",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  if (v2.size() == 0) {
    fail(u"one_of", u"at least one value is required (got: one_of(" + to_string(v1) + u")).");
    return *this;
  }`,
      to: ``,
    },
    {
      note: "错误前缀里的函数名写错",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `  _error = u"[CondMaker::" + std::u16string(func) + u"] " + msg;`,
      to: `  _error = u"[CondMaker] " + msg;`,
    },
    {
      note: "missing operator 提示里的当前文本用空串",
      file: "native/lfw/dat_translator/cond_maker.cpp",
      from: `                 u"\\" Use .and(...)/.or(...) between conditions. Current: \\"" + done() + u"\\"");`,
      to: `                 u"\\" Use .and(...)/.or(...) between conditions. Current: \\"\\"");`,
    },
  ],
};
