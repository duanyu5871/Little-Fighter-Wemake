#include "lfw/dat_translator/make_fighter_special.h"

#include <cstddef>
#include <string>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/dat_translator/fighters/fighters.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/oid.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

Value* member(Value& v, const char16_t* key) {
  Object* o = as_object(v);
  if (o == nullptr) return nullptr;
  return const_cast<Value*>(o->get(std::u16string(key)));
}

Object* base_of(Value& data) {
  Value* p = member(data, u"base");
  return p != nullptr ? as_object(*p) : nullptr;
}

using FighterFn = Value (*)(Value&);

struct FighterCase {
  const char16_t* oid;
  FighterFn fn;
};

const FighterCase* fighter_cases(size_t& count) {
  static const FighterCase cases[] = {
      {oid::kJulian, make_fighter_data_julian},
      {oid::kFirzen, make_fighter_data_firzen},
      {oid::kLouisEX, make_fighter_data_louisex},
      {oid::kBat, make_fighter_data_bat},
      {oid::kKnight, make_fighter_data_knight},
      {oid::kLouis, make_fighter_data_louis},
      {oid::kRudolf, make_fighter_data_rudolf},
      {oid::kDeep, make_fighter_data_deep},
      {oid::kDavis, make_fighter_data_davis},
      {oid::kDennis, make_fighter_data_dennis},
      {oid::kWoody, make_fighter_data_woody},
      {oid::kFiren, make_fighter_data_firen},
      {oid::kFreeze, make_fighter_data_freeze},
      {oid::kJack, make_fighter_data_jack},
      {oid::kMark, make_fighter_data_mark},
      {oid::kMonk, make_fighter_data_monk},
      {oid::kHenry, make_fighter_data_henry},
      {oid::kHunter, make_fighter_data_henter},
      {oid::kJustin, make_fighter_data_justin},
      {oid::kJan, make_fighter_data_jan},
      {oid::kSorcerer, make_fighter_data_sorcerer},
      {oid::kJohn, make_fighter_data_john},
      {oid::kTemplate, make_fighter_data_template},
  };
  count = sizeof(cases) / sizeof(cases[0]);
  return cases;
}

}

Value make_fighter_special(Value& data) {
  Object* d = as_object(data);
  if (d == nullptr) return data;
  const Value* id = d->get(u"id");
  const double num_id = to_number(id != nullptr ? *id : Value());

  if ((num_id >= 30.0 && num_id <= 39.0) || (num_id >= 50.0 && num_id <= 59.0)) {
    if (base_of(data) == nullptr) return data;
    ensure_base_group(data, entity_group::kHidden);
  }
  if (num_id >= 1.0 && num_id <= 29.0) {
    if (base_of(data) == nullptr) return data;
    ensure_base_group(data, entity_group::kRegular);
  }

  const Value* alias = d->get(u"alias_id");
  const Value selected =
      (alias != nullptr && !is_nullish(*alias)) ? *alias : (id != nullptr ? *id : Value());
  const std::u16string* key = std::get_if<std::u16string>(&selected);
  if (key == nullptr) return data;

  size_t count = 0;
  const FighterCase* cases = fighter_cases(count);
  for (size_t i = 0; i < count; ++i) {
    if (*key != cases[i].oid) continue;
    return cases[i].fn(data);
  }
  if (*key == oid::kBandit) {
    if (base_of(data) == nullptr) return data;
    ensure_base_group(data, entity_group::k_3000);
  }
  return data;
}

}
}
