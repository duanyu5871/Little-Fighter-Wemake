#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/value.h"
#include "lfw/utils/math/round_float.h"

namespace trace {

inline std::string hex16(uint64_t v) {
  char buf[17];
  std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
  return std::string(buf);
}

inline std::string bits_hex(double d) { return hex16(lfw::f64_bits(d)); }

inline std::string q_bits(double d) { return bits_hex(lfw::round_float(d)); }

inline std::string num_hex(double d) { return std::isnan(d) ? "nan" : bits_hex(d); }

class Line {
 public:
  Line& add(std::string_view v) {
    if (!_s.empty()) _s += ' ';
    _s += v;
    return *this;
  }

  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  Line& add(T v) {
    return add(std::to_string(v));
  }

  Line& add_bits(double d) { return add(bits_hex(d)); }
  Line& add_num(double d) { return add(num_hex(d)); }
  Line& add_qbits(double d) { return add(q_bits(d)); }
  Line& add_bool(bool b) { return add(b ? std::string_view("true") : std::string_view("false")); }
  Line& add_opt(bool has_value, double v) {
    return add(has_value ? q_bits(v) : std::string_view("-"));
  }

  void out() const { std::printf("%s\n", _s.c_str()); }

 private:
  std::string _s;
};

inline std::vector<std::string> split_ws(const std::string& s) {
  std::vector<std::string> out;
  size_t i = 0;
  const size_t n = s.size();
  while (i < n) {
    while (i < n && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    size_t j = i;
    while (j < n && !std::isspace(static_cast<unsigned char>(s[j]))) ++j;
    if (j > i) out.push_back(s.substr(i, j - i));
    i = j;
  }
  return out;
}

inline double to_double(const std::string& t) { return std::strtod(t.c_str(), nullptr); }

inline long to_long(const std::string& t) { return std::strtol(t.c_str(), nullptr, 10); }

inline bool to_flag(const std::string& t) { return t == "1" || t == "true"; }

inline std::u16string parse_js_string_literal(const std::string& tok) {
  std::u16string out;
  size_t i = 0;
  if (i < tok.size() && tok[i] == '"') ++i;
  for (; i < tok.size(); ++i) {
    const char c = tok[i];
    if (c == '"') break;
    if (c == '\\' && i + 1 < tok.size()) {
      const char e = tok[++i];
      if (e == 's') out.push_back(u' ');
      else if (e == 't') out.push_back(u'\t');
      else if (e == 'n') out.push_back(u'\n');
      else if (e == 'r') out.push_back(u'\r');
      else if (e == '\\') out.push_back(u'\\');
      else if (e == '"') out.push_back(u'"');
      else if (e == 'u' && i + 4 < tok.size()) {
        out.push_back(static_cast<char16_t>(
            std::strtoul(tok.substr(i + 1, 4).c_str(), nullptr, 16)));
        i += 4;
      } else {
        out.push_back(static_cast<char16_t>(static_cast<unsigned char>(e)));
      }
    } else {
      out.push_back(static_cast<char16_t>(static_cast<unsigned char>(c)));
    }
  }
  return out;
}

inline std::string to_ascii(const std::u16string& s) {
  std::string out;
  out.reserve(s.size());
  for (char16_t c : s) out.push_back(static_cast<char>(c));
  return out;
}

inline double bits_from_hex(const std::string& h) {
  return lfw::f64_from_bits(std::strtoull(h.c_str(), nullptr, 16));
}

inline std::string esc(const std::u16string& s) {
  std::string out = "\"";
  for (char16_t c : s) {
    if (c == u'"') out += "\\\"";
    else if (c == u'\\') out += "\\\\";
    else if (c < 0x20 || c > 0x7e) {
      char buf[8];
      std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(c));
      out += buf;
    } else out.push_back(static_cast<char>(c));
  }
  out += '"';
  return out;
}

inline std::u16string to_u16(const std::string& s) {
  std::u16string out;
  out.reserve(s.size());
  for (char c : s) out.push_back(static_cast<char16_t>(static_cast<unsigned char>(c)));
  return out;
}

inline lfw::Value parse_value(const std::vector<std::string>& t, size_t& i) {
  if (i >= t.size()) {
    std::fprintf(stderr, "unexpected end of value literal\n");
    std::exit(2);
  }
  const std::string kind = t[i++];
  if (kind == "u") return lfw::Value();
  if (kind == "z") return lfw::Value(lfw::NullTag{});
  if (kind == "b") return lfw::Value(t[i++] == "1");
  if (kind == "n") return lfw::Value(to_double(t[i++]));
  if (kind == "s") return lfw::Value(parse_js_string_literal(t[i++]));
  if (kind == "a") {
    const size_t n = static_cast<size_t>(to_long(t[i++]));
    auto arr = std::make_shared<lfw::Array>();
    for (size_t j = 0; j < n; ++j) arr->push_back(parse_value(t, i));
    return lfw::Value(arr);
  }
  std::fprintf(stderr, "bad value literal '%s'\n", kind.c_str());
  std::exit(2);
}

inline std::string vtag(const lfw::Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return "u";
  if (std::holds_alternative<lfw::NullTag>(v)) return "z";
  if (std::holds_alternative<bool>(v)) return "b";
  if (std::holds_alternative<double>(v)) return "n";
  if (std::holds_alternative<std::u16string>(v)) return "s";
  const lfw::Array* a = lfw::as_array(v);
  if (a != nullptr) return "a" + std::to_string(a->size());
  return "?";
}

}
