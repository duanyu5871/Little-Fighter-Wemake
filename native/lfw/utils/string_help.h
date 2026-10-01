#pragma once

#include <string>
#include <vector>

#include "lfw/core/js_string.h"

namespace lfw {

inline std::u16string js_trim(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_str_white_space(s[b])) ++b;
  while (e > b && is_str_white_space(s[e - 1])) --e;
  return s.substr(b, e - b);
}

inline std::vector<std::u16string> split_lines(const std::u16string& s) {
  std::vector<std::u16string> out;
  std::u16string cur;
  for (char16_t c : s) {
    if (c == u'\n' || c == u'\r') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

inline std::u16string replace_all(const std::u16string& s, char16_t from, char16_t to) {
  std::u16string out;
  for (char16_t c : s) out.push_back(c == from ? to : c);
  return out;
}

}
