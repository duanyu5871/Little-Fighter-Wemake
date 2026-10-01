#include "lfw/dat_translator/colon_value_reader.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_digit(char16_t c) { return c >= u'0' && c <= u'9'; }

bool is_tail(char16_t c) { return is_str_white_space(c) || c == u'|'; }

struct Match {
  bool ok = false;
  size_t start = 0;
  size_t end = 0;
  std::u16string c1;
  std::u16string c2;
};

size_t skip_spaces(const std::u16string& s, size_t p) {
  while (p < s.size() && is_str_white_space(s[p])) ++p;
  return p;
}

size_t skip_digits(const std::u16string& s, size_t p) {
  while (p < s.size() && is_digit(s[p])) ++p;
  return p;
}

Match match_at(const std::u16string& text, size_t pos, const ColonCell& cell) {
  Match m;
  if (pos + cell.name.size() > text.size()) return m;
  if (text.compare(pos, cell.name.size(), cell.name) != 0) return m;
  size_t p = skip_spaces(text, pos + cell.name.size());
  if (p >= text.size() || text[p] != u':') return m;
  p = skip_spaces(text, p + 1);

  if (cell.type == ColonCellType::kStr) {
    const size_t s = p;
    while (p < text.size() && !is_str_white_space(text[p])) ++p;
    if (p == s) return m;
    m.c1 = text.substr(s, p - s);
  } else if (cell.type == ColonCellType::kInt) {
    const size_t s = p;
    p = skip_digits(text, p);
    if (p == s) return m;
    m.c1 = text.substr(s, p - s);
  } else {
    const size_t s = p;
    const size_t d1 = skip_digits(text, p) - p;
    if (d1 == 0) return m;
    bool matched = false;
    for (size_t k = d1; k >= 1; --k) {
      const size_t q = skip_spaces(text, s + k);
      const size_t d2 = skip_digits(text, q) - q;
      if (d2 == 0) continue;
      m.c1 = text.substr(s, k);
      m.c2 = text.substr(q, d2);
      p = q + d2;
      matched = true;
      break;
    }
    if (!matched) return m;
  }

  while (p < text.size() && is_tail(text[p])) ++p;
  m.ok = true;
  m.start = pos;
  m.end = p;
  return m;
}

Match find_match(const std::u16string& text, const ColonCell& cell) {
  for (size_t pos = 0; pos < text.size(); ++pos) {
    const Match m = match_at(text, pos, cell);
    if (m.ok) return m;
  }
  return Match();
}

}

ColonValueReader& ColonValueReader::str(std::u16string name) {
  _cells.push_back(ColonCell{std::move(name), ColonCellType::kStr});
  return *this;
}

ColonValueReader& ColonValueReader::int_(std::u16string name) {
  _cells.push_back(ColonCell{std::move(name), ColonCellType::kInt});
  return *this;
}

ColonValueReader& ColonValueReader::int_2(std::u16string name) {
  _cells.push_back(ColonCell{std::move(name), ColonCellType::kIntInt});
  return *this;
}

std::u16string ColonValueReader::read(const std::u16string& text, Object& output) const {
  std::u16string rem = text;
  for (const ColonCell& cell : _cells) {
    const Match m = find_match(rem, cell);
    if (!m.ok) continue;
    if (cell.type == ColonCellType::kIntInt) {
      Array a;
      a.push_back(Value(to_number(Value(m.c1))));
      a.push_back(Value(to_number(Value(m.c2))));
      output.set(cell.name, Value(std::make_shared<Array>(a)));
    } else if (cell.type == ColonCellType::kInt) {
      output.set(cell.name, Value(to_number(Value(m.c1))));
    } else {
      output.set(cell.name, Value(m.c1));
    }
    rem = rem.substr(0, m.start) + rem.substr(m.end - m.start);
  }
  return rem;
}

}

}
