#include "lfw/entity/summary_mgr.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/defines/team_enum.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/entity/summary.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/js_add.h"

namespace lfw {
namespace {

std::u16string id_of(const Value& entity) { return to_string(field_or(entity, u"id")); }

std::size_t emitters_length(const Value& entity) {
  const Value emitters = field_or(entity, u"emitters");
  const Array* a = as_array(emitters);
  return a == nullptr ? 0 : a->size();
}

}

std::shared_ptr<Summary>* SummaryMgr::find_item(const std::u16string& id) {
  for (auto& kv : _items) {
    if (kv.first == id) return &kv.second;
  }
  return nullptr;
}

void SummaryMgr::del_item(const std::u16string& id) {
  for (std::size_t i = 0; i < _items.size(); ++i) {
    if (_items[i].first == id) {
      _items.erase(_items.begin() + static_cast<std::ptrdiff_t>(i));
      return;
    }
  }
}

std::shared_ptr<Summary> SummaryMgr::get(const std::u16string& id) {
  std::shared_ptr<Summary>* found = find_item(id);
  if (found != nullptr) return *found;
  std::shared_ptr<Summary> made = acquire(id);
  _items.push_back(std::make_pair(id, made));
  return made;
}

void SummaryMgr::clear() {
  std::vector<std::u16string> keys;
  for (const auto& kv : _items) keys.push_back(kv.first);
  for (const std::u16string& k : keys) release(k);
}

void SummaryMgr::release(const std::u16string& id) {
  std::shared_ptr<Summary>* found = find_item(id);
  if (found == nullptr) return;
  std::shared_ptr<Summary> item = *found;
  item->release();
  del_item(id);
  _graves.push_back(item);
}

std::shared_ptr<Summary> SummaryMgr::acquire(const std::u16string& id) {
  if (!_graves.empty()) {
    std::shared_ptr<Summary> ret = _graves.back();
    _graves.pop_back();
    ret->reset(id);
    return ret;
  }
  return std::make_shared<Summary>(id);
}

void SummaryMgr::add_damage_sum(const Value& a, const Value& value) {
  std::shared_ptr<Summary> self = get(id_of(a));
  self->set_damage_sum(js_add(self->damage_sum(), value));
  const std::u16string team = to_string(field_or(a, u"team"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_damage_sum(js_add(team_sum->damage_sum(), value));
  }
}

void SummaryMgr::add_kill_sum(const Value& a, const Value& value) {
  const Value amount = std::holds_alternative<std::monostate>(value) ? Value(1.0) : value;
  std::shared_ptr<Summary> self = get(id_of(a));
  self->set_kill_sum(js_add(self->kill_sum(), amount));
  const std::u16string team = to_string(field_or(a, u"team"));
  if (is_independent(team)) {
    std::shared_ptr<Summary> team_sum = get(team);
    team_sum->set_kill_sum(js_add(team_sum->kill_sum(), amount));
  }
}

void SummaryMgr::apply_damage(const Value& a, const Value& injury, const Value& v,
                              const Value& prev_hp) {
  add_damage_sum(a, injury);
  if (entity::is_fighter(v) && emitters_length(v) == 0 &&
      le(field_or(v, u"hp"), Value(0.0)) && gt(prev_hp, Value(0.0))) {
    summary_mgr().add_kill_sum(a);
  }
}

SummaryMgr& summary_mgr() {
  static SummaryMgr mgr;
  return mgr;
}

}
