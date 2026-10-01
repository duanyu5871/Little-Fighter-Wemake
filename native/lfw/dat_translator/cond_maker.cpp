#include "lfw/dat_translator/cond_maker.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/json.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

bool is_bool(const Value& v) { return std::holds_alternative<bool>(v); }
bool is_number(const Value& v) { return std::holds_alternative<double>(v); }
bool is_string(const Value& v) { return std::holds_alternative<std::u16string>(v); }

std::u16string js_trim(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_str_white_space(s[b])) ++b;
  while (e > b && is_str_white_space(s[e - 1])) --e;
  return s.substr(b, e - b);
}

std::u16string json_text(const Value& v) {
  const std::optional<std::u16string> r = json_stringify(v);
  return r.has_value() ? *r : u"undefined";
}

std::u16string type_text(const Value& v) {
  std::u16string out;
  for (const char* p = type_of(v); *p != '\0'; ++p) out.push_back(static_cast<char16_t>(*p));
  return out;
}

}

CondMaker::CondMaker() {
  _term = [](const Value& a, const std::u16string& o, const Value& b) {
    return to_string(a) + o + to_string(b);
  };
}

void CondMaker::fail(const char16_t* func, const std::u16string& msg) {
  if (!_error.empty()) return;
  _error = u"[CondMaker::" + std::u16string(func) + u"] " + msg;
}

std::u16string CondMaker::part_text(const CondPart& p) {
  return p.is_maker ? p.maker->done() : p.text;
}

unsigned CondMaker::last_kind() const {
  if (_parts.empty()) return kAllowNone;
  const CondPart& last = _parts.back();
  if (last.is_maker) return kAllowTerm;
  if (last.text == u"||") return kAllowOr;
  if (last.text == u"&&") return kAllowAnd;
  if (last.text == u"!") return kAllowNot;
  return kAllowTerm;
}

void CondMaker::opok(const char16_t* func, unsigned allow) {
  const unsigned kind = last_kind();
  if (kind != kAllowTerm && (allow & kind) != 0) return;
  const std::u16string last = _parts.empty() ? std::u16string() : part_text(_parts.back());
  fail(func, u"missing operator: \"" + last +
                 u"\" Use .and(...)/.or(...) between conditions. Current: \"" + done() + u"\"");
}

bool CondMaker::assert_value(const Value& v, const char16_t* where) {
  if (is_string(v) || is_number(v) || is_bool(v)) return true;
  fail(where, u"invalid operand: " + json_text(v) + u" (" + type_text(v) +
                 u"). Operands must be string | number | boolean.");
  return false;
}

bool CondMaker::assert_op(const std::u16string& op, const char16_t* where) {
  if (op.empty()) {
    fail(where, u"invalid operator: " + json_text(Value(op)) +
                   u". Operator must be a non-empty string.");
    return false;
  }
  if (op == u"||" || op == u"&&" || op == u"!") {
    fail(where, u"reserved operator: \"" + op +
                   u"\". \"||\" / \"&&\" / \"!\" are reserved as logical separators and cannot be "
                   u"used as comparison operators.");
    return false;
  }
  return true;
}

void CondMaker::push_text(std::u16string t) {
  CondPart p;
  p.is_maker = false;
  p.text = std::move(t);
  _parts.push_back(std::move(p));
}

void CondMaker::push_maker(CondMaker* m) {
  CondPart p;
  p.is_maker = true;
  p.maker = m;
  _parts.push_back(std::move(p));
}

CondMaker* CondMaker::make_child() {
  _children.push_back(std::make_unique<CondMaker>());
  CondMaker* c = _children.back().get();
  c->_term = _term;
  c->_quote_on = _quote_on;
  c->_quote_char = _quote_char;
  return c;
}

std::u16string CondMaker::quote_text(const std::u16string& v) const {
  if (!_quote_on) return v;
  std::u16string out(1, _quote_char);
  for (char16_t c : v) {
    if (c == _quote_char) out.push_back(u'\\');
    out.push_back(c);
  }
  out.push_back(_quote_char);
  return out;
}

CondMaker& CondMaker::quote_strings(bool on, char16_t quote) {
  if (!_error.empty()) return *this;
  if (quote != u'"' && quote != u'\'') {
    fail(u"quote_strings", u"unsupported quote character: \"" + std::u16string(1, quote) +
                               u"\". Only '\"' (double quote) or \"'\" (single quote) is allowed.");
    return *this;
  }
  _quote_on = on;
  _quote_char = quote;
  return *this;
}

CondMaker& CondMaker::term_format(CondTermFormatter fn) {
  if (!_error.empty()) return *this;
  _term = std::move(fn);
  return *this;
}

CondMaker& CondMaker::add(CondEdit fn) { return wrap(std::move(fn)); }

CondMaker& CondMaker::add(const Value& v1, const std::u16string& op, const Value& v2) {
  if (!_error.empty()) return *this;
  opok(u"add", kAllowNone | kAllowOr | kAllowAnd);
  if (!_error.empty()) return *this;
  if (is_nullish(v1) || is_nullish(v2)) {
    fail(u"add", u"incomplete comparison: missing operator or value(got: add(" + to_string(v1) +
                     u", " + op + u", " + to_string(v2) +
                     u")). Use .add(v1, op, v2) / .and(v1, op, v2) / .or(v1, op, v2). Current: \"" +
                     done() + u"\"");
    return *this;
  }
  if (!assert_value(v1, u"add")) return *this;
  if (!assert_value(v2, u"add")) return *this;
  if (!assert_op(op, u"add")) return *this;
  if (_quote_on) {
    const Value a = is_string(v1) ? Value(quote_text(std::get<std::u16string>(v1))) : v1;
    const Value b = is_string(v2) ? Value(quote_text(std::get<std::u16string>(v2))) : v2;
    push_text(_term(a, op, b));
  } else {
    push_text(_term(v1, op, v2));
  }
  return *this;
}

CondMaker& CondMaker::wrap(CondEdit fn) {
  if (!_error.empty()) return *this;
  opok(u"wrap", kAllowNone | kAllowOr | kAllowAnd | kAllowNot);
  if (!_error.empty()) return *this;
  CondMaker* child = make_child();
  CondMaker* r = fn(*child);
  CondMaker* used = r != nullptr ? r : child;
  if (!child->ok()) _error = child->error();
  push_maker(used);
  return *this;
}

CondMaker& CondMaker::not_(CondEdit fn) {
  if (!_error.empty()) return *this;
  opok(u"not", kAllowNone | kAllowOr | kAllowAnd);
  if (!_error.empty()) return *this;
  push_text(u"!");
  return wrap(std::move(fn));
}

CondMaker& CondMaker::or_(CondEdit fn) {
  if (!_error.empty()) return *this;
  if (!_parts.empty()) push_text(u"||");
  return add(std::move(fn));
}

CondMaker& CondMaker::or_(const Value& v1, const std::u16string& op, const Value& v2) {
  if (!_error.empty()) return *this;
  if (!_parts.empty()) push_text(u"||");
  return add(v1, op, v2);
}

CondMaker& CondMaker::and_(CondEdit fn) {
  if (!_error.empty()) return *this;
  if (!_parts.empty()) push_text(u"&&");
  return add(std::move(fn));
}

CondMaker& CondMaker::and_(const Value& v1, const std::u16string& op, const Value& v2) {
  if (!_error.empty()) return *this;
  if (!_parts.empty()) push_text(u"&&");
  return add(v1, op, v2);
}

CondMaker& CondMaker::one_of(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"one_of")) return *this;
  if (v2.size() == 0) {
    fail(u"one_of", u"at least one value is required (got: one_of(" + to_string(v1) + u")).");
    return *this;
  }
  opok(u"one_of", kAllowNone | kAllowOr | kAllowAnd | kAllowNot);
  if (!_error.empty()) return *this;
  const std::vector<Value> vals(v2);
  return wrap([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.or_(v1, u"==", v);
    return nullptr;
  });
}

CondMaker& CondMaker::and_one_of(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"and_one_of")) return *this;
  if (v2.size() == 0) {
    fail(u"and_one_of", u"at least one value is required (got: and_one_of(" + to_string(v1) + u")).");
    return *this;
  }
  const std::vector<Value> vals(v2);
  return and_([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.or_(v1, u"==", v);
    return nullptr;
  });
}

CondMaker& CondMaker::or_one_of(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"or_one_of")) return *this;
  if (v2.size() == 0) {
    fail(u"or_one_of", u"at least one value is required (got: or_one_of(" + to_string(v1) + u")).");
    return *this;
  }
  const std::vector<Value> vals(v2);
  return or_([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.or_(v1, u"==", v);
    return nullptr;
  });
}

CondMaker& CondMaker::not_in(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"not_in")) return *this;
  if (v2.size() == 0) {
    fail(u"not_in", u"at least one value is required (got: not_in(" + to_string(v1) + u")).");
    return *this;
  }
  opok(u"not_in", kAllowNone | kAllowOr | kAllowAnd | kAllowNot);
  if (!_error.empty()) return *this;
  const std::vector<Value> vals(v2);
  return wrap([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.and_(v1, u"!=", v);
    return nullptr;
  });
}

CondMaker& CondMaker::and_not_in(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"and_not_in")) return *this;
  if (v2.size() == 0) {
    fail(u"and_not_in",
         u"at least one value is required (got: and_not_in(" + to_string(v1) + u")).");
    return *this;
  }
  const std::vector<Value> vals(v2);
  return and_([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.and_(v1, u"!=", v);
    return nullptr;
  });
}

CondMaker& CondMaker::or_not_in(const Value& v1, const std::vector<Value>& v2) {
  if (!_error.empty()) return *this;
  if (!assert_value(v1, u"or_not_in")) return *this;
  if (v2.size() == 0) {
    fail(u"or_not_in", u"at least one value is required (got: or_not_in(" + to_string(v1) + u")).");
    return *this;
  }
  const std::vector<Value> vals(v2);
  return or_([v1, vals](CondMaker& c) -> CondMaker* {
    for (const Value& v : vals) c.and_(v1, u"!=", v);
    return nullptr;
  });
}

std::u16string CondMaker::done() const {
  if (_parts.size() == 1 && _parts[0].is_maker) return _parts[0].maker->done();
  std::u16string ret;
  for (const CondPart& p : _parts) {
    if (p.is_maker) {
      ret += u"(";
      ret += p.maker->done();
      ret += u")";
    } else {
      ret += js_trim(p.text);
    }
  }
  std::u16string out;
  out.reserve(ret.size());
  for (char16_t c : ret) {
    if (c != u'\n' && c != u'\r') out.push_back(c);
  }
  return js_trim(out);
}

}
