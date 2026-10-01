#include "lfw/dat_translator/itr_prefabs.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cookers.h"
#include "lfw/dat_translator/string_matchers.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_ws(char16_t c) { return is_str_white_space(c); }

struct EntryMatch {
  std::u16string id;
  std::u16string name;
  std::u16string remain;
};

bool match_entry_at(const std::u16string& t, size_t from, size_t& m_end, EntryMatch& out) {
  const std::u16string key = u"entry:";
  size_t scan = from;
  for (;;) {
    const size_t s = t.find(key, scan);
    if (s == std::u16string::npos) return false;
    size_t d0 = s + key.size();
    while (d0 < t.size() && is_ws(t[d0])) ++d0;
    size_t dz = d0;
    while (dz < t.size() && t[dz] >= u'0' && t[dz] <= u'9') ++dz;
    for (size_t m = dz; m > d0; --m) {
      size_t j = m;
      while (j < t.size() && is_ws(t[j])) ++j;
      if (j >= t.size() || is_ws(t[j])) continue;
      size_t jn = j;
      while (jn < t.size() && !is_ws(t[jn])) ++jn;
      size_t n2 = jn;
      while (n2 < t.size() && is_ws(t[n2])) ++n2;
      size_t re = n2;
      while (re < t.size() && t[re] != u'\n') ++re;
      out.id = t.substr(d0, m - d0);
      out.name = t.substr(j, jn - j);
      out.remain = t.substr(n2, re - n2);
      m_end = re;
      return true;
    }
    scan = s + 1;
  }
}

}

Value make_itr_prefabs(const Value& full_str) {
  const std::optional<std::u16string> body =
      match_block_once(to_string(full_str), u"<weapon_strength_list>",
                       u"<weapon_strength_list_end>");
  if (!body.has_value()) return Value();
  std::u16string text = *body;
  size_t b = 0;
  size_t e = text.size();
  while (b < e && is_ws(text[b])) ++b;
  while (e > b && is_ws(text[e - 1])) --e;
  text = text.substr(b, e - b);
  if (!is_non_empty_str(Value(text))) return Value();

  Array list;
  size_t pos = 0;
  for (;;) {
    size_t m_end = 0;
    EntryMatch m;
    if (!match_entry_at(text, pos, m_end, m)) break;
    Object entry;
    entry.set(u"kind", Value(0.0));
    entry.set(u"id", Value(m.id));
    entry.set(u"name", Value(m.name));
    const std::vector<std::pair<std::u16string, std::u16string>> kvs = match_colon_value(m.remain);
    for (const std::pair<std::u16string, std::u16string>& kv : kvs) {
      const Value v(kv.second);
      const std::optional<double> n = to_num(v);
      entry.set(kv.first, n.has_value() ? Value(*n) : v);
    }
    Value item(std::make_shared<Object>(entry));
    cook_itr(item, Value());
    list.push_back(item);
    pos = m_end;
  }
  if (list.empty()) return Value();

  Object ret;
  for (const Value& item : list.items()) {
    const Object* o = as_object(item);
    if (o == nullptr) continue;
    const Value* id = o->get(u"id");
    ret.set(id != nullptr ? to_string(*id) : std::u16string(), item);
  }
  return Value(std::make_shared<Object>(ret));
}

}
}
