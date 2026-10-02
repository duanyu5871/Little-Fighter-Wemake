#include "lfw/bot/nearest_targets.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/helper/manhattan_xz.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace bot {
namespace {

Value defendable_of(const Value& v) {
  return std::holds_alternative<std::monostate>(v) ? Value(0.0) : v;
}

bool target_less(const BotTarget& a, const BotTarget& b) {
  const double d = a.distance - b.distance;
  if (d != 0.0) {
    if (std::isnan(d)) return false;
    return d < 0.0;
  }
  const Value ia = field_or(a.entity, u"id");
  const Value ib = field_or(b.entity, u"id");
  if (lt(ia, ib)) return true;
  if (gt(ia, ib)) return false;
  return false;
}

}

Value BotTarget::facing() const { return field_or(entity, u"facing"); }

Value BotTarget::x() const { return field_or(field_or(entity, u"position"), u"x"); }

const BotTarget* NearestTargets::get() const {
  return targets_.empty() ? nullptr : &targets_[0];
}

bool NearestTargets::has_entity(const Value& v) const {
  for (const Value& e : entities_) {
    if (strict_equals(e, v)) return true;
  }
  return false;
}

void NearestTargets::add_entity(const Value& v) { entities_.push_back(v); }

void NearestTargets::del_entity(const Value& v) {
  for (std::size_t i = 0; i < entities_.size(); ++i) {
    if (strict_equals(entities_[i], v)) {
      entities_.erase(entities_.begin() + static_cast<std::ptrdiff_t>(i));
      return;
    }
  }
}

void NearestTargets::look(const Value& self, const Value& other, const Value& defendable) {
  if (!truthy(self) || has_entity(other)) return;

  const double distance = helper::manhattan_xz(self, other);
  const std::size_t len = targets_.size();
  if (static_cast<double>(len) < max_) {
    targets_.push_back(BotTarget(other, distance, defendable_of(defendable)));
    add_entity(other);
    return;
  }
  for (std::size_t i = 0; i < len; ++i) {
    if (distance > targets_[i].distance) continue;
    targets_.insert(targets_.begin() + static_cast<std::ptrdiff_t>(i),
                    BotTarget(other, distance, defendable_of(defendable)));
    add_entity(other);
    const Value entity = targets_[static_cast<std::size_t>(max_)].entity;
    del_entity(entity);
    targets_.erase(targets_.begin() + static_cast<std::ptrdiff_t>(max_), targets_.end());
    break;
  }
}

void NearestTargets::del(const std::function<bool(const BotTarget&)>& condition) {
  std::vector<BotTarget> kept;
  for (const BotTarget& t : targets_) {
    const bool ret = !condition(t);
    if (!ret) del_entity(t.entity);
    if (ret) kept.push_back(t);
  }
  targets_ = std::move(kept);
}

void NearestTargets::sort(const Value& self) {
  for (BotTarget& t : targets_) t.distance = helper::manhattan_xz(self, t.entity);
  std::stable_sort(targets_.begin(), targets_.end(), target_less);
}

void NearestTargets::clear() {
  targets_.clear();
  entities_.clear();
}

}
}
