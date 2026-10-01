#include "lfw/defines/labels.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/enum_entries.h"
#include "lfw/defines/wpoint_kind.h"

namespace lfw {
namespace defines {
namespace {

std::map<std::u16string, Value> build_enum_map(const std::vector<EnumNumberEntry>& entries) {
  std::map<std::u16string, Value> m;
  for (const EnumNumberEntry& e : entries) {
    if (e.name == nullptr) continue;
    m[std::u16string(e.name)] = Value(e.value);
    m[number_to_string(e.value)] = Value(std::u16string(e.name));
  }
  return m;
}

const std::map<std::u16string, Value>& bdy_kind_map() {
  static const std::map<std::u16string, Value> kMap = build_enum_map(bdy_kind_entries());
  return kMap;
}

const std::map<std::u16string, Value>& wpoint_kind_map() {
  static const std::map<std::u16string, Value> kMap = build_enum_map(wpoint_kind_entries());
  return kMap;
}

Value lookup_js_enum_falsy(const std::map<std::u16string, Value>& m, const Value& v) {
  std::map<std::u16string, Value>::const_iterator it = m.find(to_string(v));
  if (it != m.end() && truthy(it->second)) return it->second;
  return Value(u"unknown_" + to_string(v));
}

Value lookup_js_enum_nullish(const std::map<std::u16string, Value>& m, const Value& v) {
  std::map<std::u16string, Value>::const_iterator it = m.find(to_string(v));
  if (it != m.end() && !std::holds_alternative<std::monostate>(it->second) &&
      !std::holds_alternative<NullTag>(it->second)) {
    return it->second;
  }
  return Value(u"unknown_" + to_string(v));
}

const Object* table_object(const char16_t* name) {
  const Value* t = find(std::u16string(name));
  return t != nullptr ? as_object(*t) : nullptr;
}

const std::u16string* table_string(const char16_t* table, const std::u16string& key) {
  const Object* o = table_object(table);
  if (o == nullptr) return nullptr;
  const Value* v = o->get(key);
  return v != nullptr ? std::get_if<std::u16string>(v) : nullptr;
}

std::map<std::u16string, std::u16string>& hit_flag_memo() {
  static std::map<std::u16string, std::u16string> kMemo;
  return kMemo;
}

}

Value bdy_kind_name(const Value& v) { return lookup_js_enum_falsy(bdy_kind_map(), v); }

std::u16string bdy_kind_full_name(const Value& v) {
  return u"BdyKind." + to_string(bdy_kind_name(v));
}

Value wpoint_kind_name(const Value& v) { return lookup_js_enum_nullish(wpoint_kind_map(), v); }

std::u16string wpoint_kind_full_name(const Value& v) {
  return u"WpointKind." + to_string(wpoint_kind_name(v));
}

std::u16string get_hit_flag_name(const Value& v) {
  const double num = to_number(v);
  const std::u16string num_key = number_to_string(num);
  std::map<std::u16string, std::u16string>& memo = hit_flag_memo();
  std::map<std::u16string, std::u16string>::const_iterator it = memo.find(num_key);
  if (it != memo.end() && !it->second.empty()) return it->second;
  if (it == memo.end()) {
    const std::u16string* cached = table_string(u"HIT_FLAG_NAME_MAP", num_key);
    if (cached != nullptr && !cached->empty()) return *cached;
  }

  static const double kBase[] = {1, 2, 4, 8, 16, 32, 128};
  const int32_t vi = js_to_int32(num);
  std::u16string joined;
  for (double r : kBase) {
    if ((js_to_int32(r) & vi) == 0) continue;
    const std::u16string* part = table_string(u"HIT_FLAG_NAME_MAP", number_to_string(r));
    if (part == nullptr) continue;
    if (!joined.empty()) joined += u"|";
    joined += *part;
  }
  const std::u16string ret = joined.empty() ? (u"unknown_" + to_string(v)) : joined;
  memo[to_string(v)] = ret;
  return ret;
}

std::u16string get_hit_flag_full_name(const Value& v) {
  return u"AllyFlag." + get_hit_flag_name(v);
}

std::u16string get_hit_flag_desc(const Value& v) {
  const std::u16string* d = table_string(u"HIT_FLAG_DESC_MAP", to_string(v));
  if (d != nullptr && !d->empty()) return *d;
  return get_hit_flag_full_name(v);
}

std::map<std::u16string, std::u16string> hit_flag_name_map() {
  std::map<std::u16string, std::u16string> out;
  const Object* o = table_object(u"HIT_FLAG_NAME_MAP");
  if (o != nullptr) {
    for (const std::u16string& k : o->keys()) {
      const Value* v = o->get(k);
      if (v == nullptr) continue;
      if (const std::u16string* s = std::get_if<std::u16string>(v)) out[k] = *s;
    }
  }
  for (const std::pair<const std::u16string, std::u16string>& kv : hit_flag_memo()) {
    out[kv.first] = kv.second;
  }
  return out;
}

}

}
