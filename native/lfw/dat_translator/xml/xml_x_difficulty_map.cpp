#include "lfw/dat_translator/xml/xml_x_difficulty_map.h"

#include <cmath>
#include <variant>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/defines/defines_data.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

bool is_nullish_or_absent(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::vector<std::u16string> split_char(const std::u16string& s, char16_t sep) {
  std::vector<std::u16string> ret;
  std::u16string cur;
  for (char16_t c : s) {
    if (c == sep) {
      ret.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  ret.push_back(cur);
  return ret;
}

}  // namespace

void xml_x_difficulty_map(const std::shared_ptr<IXMLElement>& el, const std::u16string& attr,
                          const Value& map) {
  if (is_nullish_or_absent(map) || !truthy(map)) return;
  const Object* const m = as_object(map);
  if (m == nullptr) return;
  const Value* const list_v = defines::find(u"DifficultyList");
  const Array* const list = list_v != nullptr ? as_array(*list_v) : nullptr;
  std::vector<std::u16string> parts;
  if (list != nullptr) {
    for (size_t i = 0; i < list->size(); ++i) {
      const double* const k = std::get_if<double>(&list->at(i));
      if (k == nullptr) continue;
      const std::u16string key = number_to_string(*k);
      if (!m->has(key)) continue;
      const Value* const mv = m->get(key);
      const double* const mv_num = mv != nullptr ? std::get_if<double>(mv) : nullptr;
      if (mv_num == nullptr) continue;
      parts.push_back(key + u":" + number_to_string(*mv_num));
    }
  }
  std::u16string value;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) value.push_back(u',');
    value += parts[i];
  }
  if (value.empty()) return;
  el->set_attr(attr, Value(std::move(value)));
}

Value xml_2_difficulty_map(const IXMLElement& el, const std::u16string& attr) {
  const std::optional<std::u16string> v = el.attr(attr);
  if (!v || v->empty()) return Value();
  auto ret = std::make_shared<Object>();
  for (const std::u16string& str : split_char(*v, u',')) {
    const std::vector<std::u16string> segs = split_char(str, u':');
    const double k = string_to_number(segs[0]);
    const double val = segs.size() >= 2 ? string_to_number(segs[1]) : std::nan("");
    if (std::isnan(k) || std::isnan(val)) continue;
    ret->set(number_to_string(k), Value(val));
  }
  if (ret->keys().empty()) return Value();
  return Value(std::move(ret));
}

}
}
}
