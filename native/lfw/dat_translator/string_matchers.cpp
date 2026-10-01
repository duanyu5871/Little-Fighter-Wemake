#include "lfw/dat_translator/string_matchers.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_ws(char16_t c) { return is_str_white_space(c); }

std::u16string trimmed(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_ws(s[b])) ++b;
  while (e > b && is_ws(s[e - 1])) --e;
  return s.substr(b, e - b);
}

bool match_colon_at(const std::u16string& t, size_t i, std::pair<std::u16string, std::u16string>& out,
                    size_t& end_pos) {
  size_t p = i;
  while (p < t.size() && is_ws(t[p])) ++p;
  const size_t key_start = p;
  size_t key_end = p;
  while (key_end < t.size() && !is_ws(t[key_end])) ++key_end;
  size_t klen = key_end - key_start;
  for (;;) {
    const size_t after_key = key_start + klen;
    size_t j = after_key;
    while (j < t.size() && is_ws(t[j])) ++j;
    if (j < t.size() && t[j] == u':') {
      size_t v_start = j + 1;
      while (v_start < t.size() && is_ws(t[v_start])) ++v_start;
      size_t v_end = v_start;
      while (v_end < t.size() && !is_ws(t[v_end])) ++v_end;
      out = std::make_pair(t.substr(key_start, klen), t.substr(v_start, v_end - v_start));
      end_pos = v_end;
      return true;
    }
    if (klen == 0) break;
    --klen;
  }
  return false;
}

bool find_block(const std::u16string& t, const std::u16string& start, const std::u16string& end,
                size_t from, size_t& m_start, size_t& m_end, size_t& body_start, size_t& body_end) {
  size_t scan = from;
  for (;;) {
    const size_t s = t.find(start, scan);
    if (s == std::u16string::npos) return false;
    const size_t body_from = s + start.size();
    if (body_from < t.size()) {
      const size_t e = t.find(end, body_from + 1);
      if (e != std::u16string::npos) {
        m_start = s;
        m_end = e + end.size();
        body_start = body_from;
        body_end = e;
        return true;
      }
    }
    scan = s + 1;
  }
}

bool find_block_trimmed(const std::u16string& t, const std::u16string& start,
                        const std::u16string& end, size_t from, size_t& m_start, size_t& m_end,
                        size_t& body_start, size_t& body_end) {
  return find_block(t, trimmed(start), trimmed(end), from, m_start, m_end, body_start, body_end);
}

}

std::vector<std::pair<std::u16string, std::u16string>> match_colon_value(const std::u16string& text) {
  const std::u16string t = trimmed(text);
  std::vector<std::pair<std::u16string, std::u16string>> out;
  size_t pos = 0;
  while (pos <= t.size()) {
    bool hit = false;
    for (size_t i = pos; i <= t.size(); ++i) {
      std::pair<std::u16string, std::u16string> kv;
      size_t end_pos = 0;
      if (match_colon_at(t, i, kv, end_pos)) {
        out.push_back(kv);
        pos = end_pos > pos ? end_pos : pos + 1;
        hit = true;
        break;
      }
    }
    if (!hit) break;
  }
  return out;
}

std::optional<std::u16string> match_block_once(const std::u16string& text,
                                               const std::u16string& start,
                                               const std::u16string& end) {
  size_t m_start = 0;
  size_t m_end = 0;
  size_t body_start = 0;
  size_t body_end = 0;
  if (!find_block_trimmed(text, start, end, 0, m_start, m_end, body_start, body_end)) {
    return std::nullopt;
  }
  return text.substr(body_start, body_end - body_start);
}

TakeBlocksResult take_blocks(const std::u16string& text, const std::u16string& start,
                             const std::u16string& end) {
  TakeBlocksResult ret;
  std::vector<std::pair<size_t, size_t>> positions;
  size_t from = 0;
  size_t m_start = 0;
  size_t m_end = 0;
  size_t body_start = 0;
  size_t body_end = 0;
  while (find_block_trimmed(text, start, end, from, m_start, m_end, body_start, body_end)) {
    positions.push_back(std::make_pair(m_start, m_end));
    ret.blocks.push_back(text.substr(body_start, body_end - body_start));
    from = m_end;
  }
  if (positions.empty()) {
    ret.remains = text;
    return ret;
  }
  std::u16string remains;
  size_t prev = 0;
  for (const std::pair<size_t, size_t>& p : positions) {
    remains += text.substr(prev, p.first - prev);
    prev = p.second;
  }
  remains += text.substr(prev);
  ret.remains = remains;
  return ret;
}

std::optional<std::u16string> match_hash_end(const std::u16string& text) {
  const size_t pos = text.find(u'#');
  if (pos == std::u16string::npos) return std::nullopt;
  std::u16string out;
  for (size_t i = pos + 1; i < text.size(); ++i) {
    const char16_t c = text[i];
    if (c == u'\n' || c == u'\r' || c == u'\u2028' || c == u'\u2029') break;
    out.push_back(c);
  }
  return out;
}

TakeSectionsResult take_sections(const std::u16string& text, const std::u16string& start,
                                 const std::u16string& end) {
  const TakeBlocksResult blocks = take_blocks(text, start, end);
  TakeSectionsResult ret;
  for (const std::u16string& block : blocks.blocks) {
    Object item;
    for (const std::pair<std::u16string, std::u16string>& kv : match_colon_value(block)) {
      const double num = to_number(Value(kv.second));
      if (is_num(num)) item.set(kv.first, Value(num));
      else item.set(kv.first, Value(kv.second));
    }
    ret.sections.push_back(Value(std::make_shared<Object>(item)));
  }
  ret.remains = blocks.remains;
  return ret;
}

}
}
