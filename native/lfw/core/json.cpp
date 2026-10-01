#include "json.h"

#include <cmath>
#include <cstddef>
#include <memory>

#include "js_string.h"

namespace lfw {

namespace {

const char16_t* const kHexDigits = u"0123456789abcdef";

void quote(const std::u16string& s, std::u16string& out) {
  out.push_back(u'"');
  for (char16_t c : s) {
    switch (c) {
      case u'"':
        out += u"\\\"";
        break;
      case u'\\':
        out += u"\\\\";
        break;
      case 0x0008:
        out += u"\\b";
        break;
      case 0x000c:
        out += u"\\f";
        break;
      case 0x000a:
        out += u"\\n";
        break;
      case 0x000d:
        out += u"\\r";
        break;
      case 0x0009:
        out += u"\\t";
        break;
      default:
        if (c < 0x20) {
          out += u"\\u";
          out.push_back(kHexDigits[(c >> 12) & 0xf]);
          out.push_back(kHexDigits[(c >> 8) & 0xf]);
          out.push_back(kHexDigits[(c >> 4) & 0xf]);
          out.push_back(kHexDigits[c & 0xf]);
        } else {
          out.push_back(c);
        }
    }
  }
  out.push_back(u'"');
}

bool write(const Value& v, std::u16string& out) {
  if (std::holds_alternative<std::monostate>(v)) return false;
  if (std::holds_alternative<NullTag>(v)) {
    out += u"null";
    return true;
  }
  if (const bool* b = std::get_if<bool>(&v)) {
    out += *b ? u"true" : u"false";
    return true;
  }
  if (const double* d = std::get_if<double>(&v)) {
    out += std::isfinite(*d) ? number_to_string(*d) : std::u16string(u"null");
    return true;
  }
  if (const std::u16string* s = std::get_if<std::u16string>(&v)) {
    quote(*s, out);
    return true;
  }
  if (const Array* a = as_array(v)) {
    out.push_back(u'[');
    for (size_t i = 0; i < a->size(); ++i) {
      if (i != 0) out.push_back(u',');
      const Value& item = a->at(i);
      if (std::holds_alternative<std::monostate>(item)) {
        out += u"null";
        continue;
      }
      write(item, out);
    }
    out.push_back(u']');
    return true;
  }
  const Object* o = as_object(v);
  if (o == nullptr) return false;
  out.push_back(u'{');
  bool first = true;
  for (const std::u16string& k : o->keys()) {
    const Value* p = o->get(k);
    if (p == nullptr || std::holds_alternative<std::monostate>(*p)) continue;
    if (!first) out.push_back(u',');
    first = false;
    quote(k, out);
    out.push_back(u':');
    write(*p, out);
  }
  out.push_back(u'}');
  return true;
}

bool is_json_ws(char16_t c) {
  return c == u' ' || c == u'\t' || c == u'\n' || c == u'\r';
}

struct Parser {
  const std::u16string& s;
  size_t i = 0;
  bool ok = true;

  void ws() {
    while (i < s.size() && is_json_ws(s[i])) ++i;
  }

  bool at(const char16_t* w, size_t n) const { return s.compare(i, n, w) == 0; }

  bool digit() { return i < s.size() && s[i] >= u'0' && s[i] <= u'9'; }

  Value fail() {
    ok = false;
    return Value();
  }

  std::u16string str() {
    std::u16string out;
    if (!ok || i >= s.size() || s[i] != u'"') {
      fail();
      return out;
    }
    ++i;
    while (i < s.size()) {
      const char16_t c = s[i++];
      if (c == u'"') return out;
      if (c != u'\\') {
        out.push_back(c);
        continue;
      }
      if (i >= s.size()) {
        fail();
        return out;
      }
      const char16_t e = s[i++];
      switch (e) {
        case u'"':
          out.push_back(u'"');
          break;
        case u'\\':
          out.push_back(u'\\');
          break;
        case u'/':
          out.push_back(u'/');
          break;
        case u'b':
          out.push_back(0x0008);
          break;
        case u'f':
          out.push_back(0x000c);
          break;
        case u'n':
          out.push_back(0x000a);
          break;
        case u'r':
          out.push_back(0x000d);
          break;
        case u't':
          out.push_back(0x0009);
          break;
        case u'u': {
          if (i + 4 > s.size()) {
            fail();
            return out;
          }
          unsigned v = 0;
          for (size_t k = 0; k < 4; ++k) {
            const char16_t h = s[i + k];
            unsigned d = 0;
            if (h >= u'0' && h <= u'9') d = static_cast<unsigned>(h - u'0');
            else if (h >= u'a' && h <= u'f') d = static_cast<unsigned>(h - u'a') + 10;
            else if (h >= u'A' && h <= u'F') d = static_cast<unsigned>(h - u'A') + 10;
            else {
              fail();
              return out;
            }
            v = v * 16 + d;
          }
          i += 4;
          out.push_back(static_cast<char16_t>(v));
          break;
        }
        default:
          fail();
          return out;
      }
    }
    fail();
    return out;
  }

  Value num() {
    const size_t start = i;
    if (i < s.size() && s[i] == u'-') ++i;
    if (!digit()) return fail();
    if (s[i] == u'0') {
      ++i;
    } else {
      while (digit()) ++i;
    }
    if (i < s.size() && s[i] == u'.') {
      ++i;
      if (!digit()) return fail();
      while (digit()) ++i;
    }
    if (i < s.size() && (s[i] == u'e' || s[i] == u'E')) {
      ++i;
      if (i < s.size() && (s[i] == u'+' || s[i] == u'-')) ++i;
      if (!digit()) return fail();
      while (digit()) ++i;
    }
    return Value(string_to_number(s.substr(start, i - start)));
  }

  Value arr() {
    ++i;
    auto a = std::make_shared<Array>();
    ws();
    if (i < s.size() && s[i] == u']') {
      ++i;
      return Value(a);
    }
    for (;;) {
      ws();
      a->push_back(val());
      if (!ok) return Value();
      ws();
      if (i < s.size() && s[i] == u',') {
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == u']') {
        ++i;
        return Value(a);
      }
      return fail();
    }
  }

  Value obj() {
    ++i;
    auto o = std::make_shared<Object>();
    ws();
    if (i < s.size() && s[i] == u'}') {
      ++i;
      return Value(o);
    }
    for (;;) {
      ws();
      if (i >= s.size() || s[i] != u'"') return fail();
      const std::u16string k = str();
      if (!ok) return Value();
      ws();
      if (i >= s.size() || s[i] != u':') return fail();
      ++i;
      ws();
      o->set(k, val());
      if (!ok) return Value();
      ws();
      if (i < s.size() && s[i] == u',') {
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == u'}') {
        ++i;
        return Value(o);
      }
      return fail();
    }
  }

  Value val() {
    if (!ok || i >= s.size()) return fail();
    const char16_t c = s[i];
    if (c == u'{') return obj();
    if (c == u'[') return arr();
    if (c == u'"') return Value(str());
    if (c == u't') {
      if (!at(u"true", 4)) return fail();
      i += 4;
      return Value(true);
    }
    if (c == u'f') {
      if (!at(u"false", 5)) return fail();
      i += 5;
      return Value(false);
    }
    if (c == u'n') {
      if (!at(u"null", 4)) return fail();
      i += 4;
      return Value(NullTag{});
    }
    return num();
  }
};

}

std::optional<std::u16string> json_stringify(const Value& v) {
  std::u16string out;
  if (!write(v, out)) return std::nullopt;
  return out;
}

std::optional<Value> json_parse(const std::u16string& text) {
  Parser p{text};
  p.ws();
  Value v = p.val();
  if (!p.ok) return std::nullopt;
  p.ws();
  if (p.i != text.size()) return std::nullopt;
  return v;
}

}
