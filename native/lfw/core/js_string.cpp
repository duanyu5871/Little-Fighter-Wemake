#include "js_string.h"

#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace lfw {
namespace {

double nan_v() { return std::numeric_limits<double>::quiet_NaN(); }

}

bool is_str_white_space(char16_t c) {
  switch (c) {
    case 0x0009:
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
      return true;
    default:
      return c >= 0x2000 && c <= 0x200a;
  }
}

namespace {

int digit_value(char16_t c) {
  if (c >= u'0' && c <= u'9') return static_cast<int>(c - u'0');
  if (c >= u'a' && c <= u'f') return static_cast<int>(c - u'a') + 10;
  if (c >= u'A' && c <= u'F') return static_cast<int>(c - u'A') + 10;
  return -1;
}

double parse_radix(const std::u16string& s, size_t from, uint64_t base, int log2base) {
  const size_t n = s.size();
  if (from >= n) return nan_v();

  const uint64_t mask = (uint64_t(1) << log2base) - 1;
  uint64_t acc = 0;
  size_t shifted = 0;
  bool sticky = false;

  for (size_t i = from; i < n; ++i) {
    const int d = digit_value(s[i]);
    if (d < 0 || static_cast<uint64_t>(d) >= base) return nan_v();
    if ((acc >> (64 - log2base)) != 0) {
      if ((acc & mask) != 0) sticky = true;
      acc >>= log2base;
      shifted += static_cast<size_t>(log2base);
    }
    acc = acc * base + static_cast<uint64_t>(d);
  }

  if (acc == 0) return 0.0;
  if (shifted > 1100) return std::numeric_limits<double>::infinity();
  if (shifted == 0) return static_cast<double>(acc);

  const int lead = 63 - std::countl_zero(acc);
  const int drop = lead - 52;
  if (drop <= 0) return std::ldexp(static_cast<double>(acc), static_cast<int>(shifted));

  const uint64_t keep = acc >> drop;
  const uint64_t rest = acc & ((uint64_t(1) << drop) - 1);
  const uint64_t half = uint64_t(1) << (drop - 1);
  uint64_t rounded = keep;
  if (rest > half || (rest == half && ((keep & 1) != 0 || sticky))) ++rounded;
  return std::ldexp(static_cast<double>(rounded), drop + static_cast<int>(shifted));
}

struct DecimalScan {
  bool ok = false;
  bool neg = false;
  bool infinity = false;
  int64_t exp = 0;
  size_t int_digits = 0;
  size_t first_sig = static_cast<size_t>(-1);
};

DecimalScan scan_decimal(const std::u16string& s) {
  DecimalScan r;
  const size_t n = s.size();
  size_t i = 0;

  if (i < n && (s[i] == u'+' || s[i] == u'-')) {
    r.neg = (s[i] == u'-');
    ++i;
  }

  if (n - i == 8) {
    bool same = true;
    for (size_t k = 0; k < 8; ++k) {
      if (s[i + k] != u"Infinity"[k]) {
        same = false;
        break;
      }
    }
    if (same) {
      r.ok = true;
      r.infinity = true;
      return r;
    }
  }

  bool any = false;
  size_t sig_index = 0;

  while (i < n && s[i] >= u'0' && s[i] <= u'9') {
    if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;
    ++sig_index;
    ++r.int_digits;
    ++i;
    any = true;
  }
  if (i < n && s[i] == u'.') {
    ++i;
    while (i < n && s[i] >= u'0' && s[i] <= u'9') {
      if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;
      ++sig_index;
      ++i;
      any = true;
    }
  }
  if (!any) return r;

  if (i < n && (s[i] == u'e' || s[i] == u'E')) {
    ++i;
    bool eneg = false;
    if (i < n && (s[i] == u'+' || s[i] == u'-')) {
      eneg = (s[i] == u'-');
      ++i;
    }
    bool edig = false;
    int64_t e = 0;
    while (i < n && s[i] >= u'0' && s[i] <= u'9') {
      if (e < 1000000) e = e * 10 + static_cast<int64_t>(s[i] - u'0');
      ++i;
      edig = true;
    }
    if (!edig) return r;
    r.exp = eneg ? -e : e;
  }

  if (i != n) return r;
  r.ok = true;
  return r;
}

}

double string_to_number(const std::u16string& raw) {
  size_t b = 0;
  size_t e = raw.size();
  while (b < e && is_str_white_space(raw[b])) ++b;
  while (e > b && is_str_white_space(raw[e - 1])) --e;
  if (b == e) return 0.0;

  const std::u16string s = raw.substr(b, e - b);
  const size_t n = s.size();

  if (n >= 2 && s[0] == u'0') {
    const char16_t p = s[1];
    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 4);
    if (p == u'o' || p == u'O') return parse_radix(s, 2, 8, 3);
    if (p == u'b' || p == u'B') return parse_radix(s, 2, 2, 1);
  }

  const DecimalScan sc = scan_decimal(s);
  if (!sc.ok) return nan_v();
  if (sc.infinity) return sc.neg ? -std::numeric_limits<double>::infinity()
                                 : std::numeric_limits<double>::infinity();

  std::string ascii;
  ascii.reserve(n);
  for (char16_t c : s) ascii.push_back(static_cast<char>(c));

  const char* begin = ascii.c_str();
  if (*begin == '+') ++begin;
  const char* end = ascii.c_str() + n;

  double v = 0.0;
  const std::from_chars_result r = std::from_chars(begin, end, v, std::chars_format::general);
  if (r.ec == std::errc::result_out_of_range) {
    const int64_t e10 = static_cast<int64_t>(sc.int_digits) - 1 -
                        static_cast<int64_t>(sc.first_sig) + sc.exp;
    const double mag = e10 >= 0 ? std::numeric_limits<double>::infinity() : 0.0;
    return sc.neg ? -mag : mag;
  }
  if (r.ec != std::errc() || r.ptr != end) return nan_v();
  return v;
}

namespace {

bool shortest_digits(double v, std::string& digits, int& exp10) {
  char buf[40];
  const std::to_chars_result r =
      std::to_chars(buf, buf + sizeof buf, v, std::chars_format::scientific);
  if (r.ec != std::errc()) return false;
  const size_t n = static_cast<size_t>(r.ptr - buf);

  digits.clear();
  size_t i = 0;
  if (i < n && buf[i] == '-') ++i;
  digits.push_back(buf[i++]);
  if (i < n && buf[i] == '.') {
    ++i;
    while (i < n && buf[i] != 'e') digits.push_back(buf[i++]);
  }

  int e = 0;
  bool eneg = false;
  if (i < n && buf[i] == 'e') {
    ++i;
    if (i < n && (buf[i] == '+' || buf[i] == '-')) {
      eneg = (buf[i] == '-');
      ++i;
    }
    while (i < n) e = e * 10 + static_cast<int>(buf[i++] - '0');
  }
  exp10 = eneg ? -e : e;
  return true;
}

}

std::u16string number_to_string(double v) {
  if (std::isnan(v)) return u"NaN";
  if (v == 0.0) return u"0";
  if (std::isinf(v)) return v < 0 ? u"-Infinity" : u"Infinity";

  const bool neg = v < 0;
  const double a = neg ? -v : v;

  std::string d;
  int e10 = 0;
  if (!shortest_digits(a, d, e10)) return u"NaN";

  const int k = static_cast<int>(d.size());
  const int n = e10 + 1;

  std::u16string out;
  if (neg) out.push_back(u'-');

  const auto push_digits = [&out](const char* p, size_t count) {
    for (size_t j = 0; j < count; ++j) out.push_back(static_cast<char16_t>(p[j]));
  };

  if (k <= n && n <= 21) {
    push_digits(d.data(), d.size());
    out.append(static_cast<size_t>(n - k), u'0');
  } else if (n > 0 && n <= 21) {
    push_digits(d.data(), static_cast<size_t>(n));
    out.push_back(u'.');
    push_digits(d.data() + n, d.size() - static_cast<size_t>(n));
  } else if (n > -6 && n <= 0) {
    out.push_back(u'0');
    out.push_back(u'.');
    out.append(static_cast<size_t>(-n), u'0');
    push_digits(d.data(), d.size());
  } else {
    push_digits(d.data(), 1);
    if (k > 1) {
      out.push_back(u'.');
      push_digits(d.data() + 1, d.size() - 1);
    }
    out.push_back(u'e');
    const int ex = n - 1;
    out.push_back(ex < 0 ? u'-' : u'+');
    int ax = ex < 0 ? -ex : ex;
    char tmp[12];
    int ti = 0;
    do {
      tmp[ti++] = static_cast<char>('0' + ax % 10);
      ax /= 10;
    } while (ax > 0);
    while (ti > 0) out.push_back(static_cast<char16_t>(tmp[--ti]));
  }

  return out;
}

}
