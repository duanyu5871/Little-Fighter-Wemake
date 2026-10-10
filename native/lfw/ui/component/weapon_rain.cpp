#include "lfw/ui/component/weapon_rain.h"

#include <algorithm>
#include <cmath>

#include "lfw/core/js_string.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/entity_group.h"
#include "lfw/entity/entity.h"
#include "lfw/lfw.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/mersenne_twister.h"
#include "lfw/utils/string_help.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

constexpr double kDefaultChance = 0.08;
constexpr double kDefaultPower = 1.5;
constexpr double kDefaultLimit = 3;
constexpr double kRollScale = 20000;

std::optional<std::u16string> prop_str(const Value* const props, const char16_t* key) {
  if (props == nullptr) return std::nullopt;
  const Object* const o = as_object(*props);
  const Value* const v = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  if (v == nullptr) return std::nullopt;
  const std::u16string* const s = std::get_if<std::u16string>(v);
  return s != nullptr ? std::optional<std::u16string>(*s) : std::nullopt;
}

std::optional<double> prop_num(const Value* const props, const char16_t* key) {
  if (props == nullptr) return std::nullopt;
  const Object* const o = as_object(*props);
  const Value* const v = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  if (v == nullptr) return std::nullopt;
  const double* const n = std::get_if<double>(v);
  return n != nullptr ? std::optional<double>(*n) : std::nullopt;
}

}

const ClazzTag* WeaponRain::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& WeaponRain::TAGS() {
  static const std::vector<std::u16string> tags{u"WeaponRain"};
  return tags;
}

const std::u16string& WeaponRain::props_tag() const {
  static const std::u16string tag = u"WeaponRain";
  return tag;
}

const Value& WeaponRain::props_meta() const {
  static const Value meta = [] {
    auto str_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"string"));
      p->set(u"nullable", Value(true));
      return p;
    };
    auto num_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"number"));
      p->set(u"nullable", Value(true));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"oids", Value(str_prop()));
    o->set(u"groups", Value(str_prop()));
    o->set(u"chance", Value(num_prop()));
    o->set(u"interval", Value(num_prop()));
    o->set(u"power", Value(num_prop()));
    o->set(u"limit", Value(num_prop()));
    return Value(o);
  }();
  return meta;
}

void WeaponRain::on_start() {
  const Value* const p = props();
  const std::optional<double> interval = prop_num(p, u"interval");
  if (interval.has_value()) timer.set_range(0, *interval);
}

void WeaponRain::update(double dt) {
  if (world().paused()) return;
  Stage* const st = world().stage();
  if (st != nullptr && st->weapon_rain_disabled()) return;
  if (!timer.add(dt)) return;
  try_drop_weapon();
}

void WeaponRain::try_drop_weapon() {
  World& w = world();
  const Value* const p = props();
  const std::u16string groups =
      prop_str(p, u"groups").value_or(std::u16string(entity_group::kVsWeapon));
  const double chance = prop_num(p, u"chance").value_or(kDefaultChance);
  const double power = prop_num(p, u"power").value_or(kDefaultPower);
  MersenneTwister& mt = lfw().mt_ref();
  const double limit = prop_num(p, u"limit").value_or(kDefaultLimit);
  mt.mark = u"weapon_rain_range";
  const double x = w.random_weapon_x(last_section);
  last_section = w.weapon_section_at(x);
  const double count = w.weapon_count_at(x);
  mt.mark = u"weapon_rain_chance";
  const double left = limit > 0 ? lfw::max(0.0, 1.0 - count / limit) : 0.0;
  const double threshold = lfw::floor(chance * lfw::pow(left, power) * kRollScale);
  if (mt.range(0, kRollScale) >= threshold) return;
  Entity* const entity = drop_weapon(groups);
  if (entity == nullptr) return;
  entity->set_position(Value(x), Value(), Value());
}

Entity* WeaponRain::drop_weapon(const std::u16string& groups) {
  const std::vector<const Value*>& datas = resolve_oid_datas();
  if (!datas.empty()) {
    MersenneTwister& mt = lfw().mt_ref();
    mt.mark = u"weapon_rain_oids";
    const size_t idx = static_cast<size_t>(mt.range(0, static_cast<double>(datas.size())));
    std::vector<Entity*> const added = lfw().weapons_helper().add(Value(*datas[idx]), 1);
    return added.empty() ? nullptr : added[0];
  }
  std::vector<Entity*> const added = lfw().weapons_helper().add_random(1, true, groups);
  return added.empty() ? nullptr : added[0];
}

const std::vector<const Value*>& WeaponRain::resolve_oid_datas() {
  const std::optional<std::u16string> oids = prop_str(props(), u"oids");
  if (_using_oids.has_value() == oids.has_value() &&
      (!oids.has_value() || *_using_oids == *oids)) {
    return _oid_datas;
  }
  _using_oids = oids;
  _oid_datas.clear();
  if (oids.has_value()) {
    size_t start = 0;
    const std::u16string& text = *oids;
    while (start <= text.size()) {
      const size_t comma = text.find(u',', start);
      const size_t end = comma == std::u16string::npos ? text.size() : comma;
      const std::u16string oid = js_trim(text.substr(start, end - start));
      start = end + 1;
      if (oid.empty()) continue;
      const Value* const data = lfw().find_weapon(Value(oid));
      if (data != nullptr) {
        _oid_datas.push_back(data);
        continue;
      }
      if (_warned_oids.count(oid) == 0) {
        _warned_oids.insert(oid);
        lfw().host().warn({Value(u"[WeaponRain] oid not found: " + oid)});
      }
    }
  }
  return _oid_datas;
}

}
}
