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
#include "lfw/utils/math/round_float.h"

namespace trace {

inline std::string hex16(uint64_t v) {
  char buf[17];
  std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
  return std::string(buf);
}

inline std::string bits_hex(double d) { return hex16(lfw::f64_bits(d)); }

inline std::string q_bits(double d) { return bits_hex(lfw::round_float(d)); }

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

}
