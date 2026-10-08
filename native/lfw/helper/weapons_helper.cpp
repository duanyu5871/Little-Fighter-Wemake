#include "lfw/helper/weapons_helper.h"

#include <memory>
#include <utility>

#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace helper {

namespace {

// `String.prototype.trim()` 的常用面（空白 + NBSP + BOM）。
bool is_js_ws(char16_t c) {
  return c == u' ' || c == u'\t' || c == u'\n' || c == u'\r' || c == u'\v' || c == u'\f' ||
         c == u'\u00a0' || c == u'\ufeff';
}

std::u16string js_trim(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_js_ws(s[b])) ++b;
  while (e > b && is_js_ws(s[e - 1])) --e;
  return s.substr(b, e - b);
}

std::vector<std::u16string> split_comma(const std::u16string& s) {
  std::vector<std::u16string> ret;
  std::u16string cur;
  for (const char16_t c : s) {
    if (c == u',') {
      ret.push_back(std::move(cur));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  ret.push_back(std::move(cur));
  return ret;
}

// `v.base.group?.some(a => gg.includes(a))`：`base`/`group` 缺或不是数组 ⇒ 不入选
// （TS 在「`group` 存在但不是数组」时会抛 TypeError，这里按不入选处理，见 DESIGN）。
bool in_groups(const Value& v, const std::vector<std::u16string>& gg) {
  const Value* const group = [&v]() -> const Value* {
    const Object* const base = as_object(field_or(v, u"base"));
    return base != nullptr ? base->get(u"group") : nullptr;
  }();
  const Array* const arr = group != nullptr ? as_array(*group) : nullptr;
  if (arr == nullptr) return false;
  for (size_t i = 0; i < arr->size(); ++i) {
    const std::u16string* const item = std::get_if<std::u16string>(&arr->at(i));
    if (item == nullptr) continue;
    for (const std::u16string& g : gg) {
      if (g == *item) return true;
    }
  }
  return false;
}

}  // namespace

std::vector<Entity*> WeaponsHelper::all() const {
  std::vector<Entity*> ret;
  for (Entity* const v : _lfw->world_entities()) {
    if (entity::is_weapon_data(v->data())) ret.push_back(v);
  }
  for (Entity* const v : _lfw->world_ghosts()) {
    if (entity::is_weapon_data(v->data())) ret.push_back(v);
  }
  return ret;
}

std::vector<Entity*> WeaponsHelper::add(const Value& data, double num, const std::u16string* team) {
  Value d = data;
  if (std::get_if<std::u16string>(&d) != nullptr) {
    const Value* const found = _lfw->find_weapon(d);
    if (found == nullptr) return {};
    d = *found;
  }
  if (!truthy(d)) return {};
  return ObjectsHelper::add(d, num, team);
}

RandomingT<Value>* WeaponsHelper::randoms(const std::u16string& groups, bool duplicate) {
  std::vector<std::pair<std::u16string, RandomingT<Value>::Ptr>>& map =
      duplicate ? _random_d_map : _random_map;
  for (const auto& kv : map) {
    if (kv.first == groups) return kv.second.get();
  }
  std::vector<Value> list = _lfw->weapons();
  std::u16string name = u"weapons_randoms";
  if (!groups.empty()) {
    std::vector<std::u16string> gg;
    for (std::u16string part : split_comma(groups)) gg.push_back(js_trim(part));
    name.push_back(u'_');
    for (size_t i = 0; i < gg.size(); ++i) {
      if (i != 0) name.push_back(u'_');
      name += gg[i];
    }
    std::vector<Value> filtered;
    for (const Value& v : list) {
      if (in_groups(v, gg)) filtered.push_back(v);
    }
    list = std::move(filtered);
  }
  if (list.empty()) return nullptr;
  RandomingT<Value>::Ptr ret =
      std::make_shared<RandomingT<Value>>(name, std::move(list), &_lfw->mt(), Value(duplicate));
  map.emplace_back(groups, ret);
  return ret.get();
}

std::vector<Entity*> WeaponsHelper::add_random(double num, bool duplicate,
                                               const std::u16string& group) {
  RandomingT<Value>* const randoms_v = randoms(group, duplicate);
  std::vector<Entity*> ret;
  if (randoms_v == nullptr) return ret;
  for (double n = num;;) {
    n -= 1;
    if (!(n >= 0)) break;
    const Value d = randoms_v->get();
    if (!truthy(d)) continue;
    const std::vector<Entity*> added = add(d, 1, nullptr);
    ret.insert(ret.end(), added.begin(), added.end());
  }
  return ret;
}

}  // namespace helper
}  // namespace lfw
