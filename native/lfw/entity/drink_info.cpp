#include "lfw/entity/drink_info.h"

#include <array>
#include <cstddef>
#include <memory>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace {

Value or_default(const Value& v, const Value& fallback) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v) ? fallback
                                                                                        : v;
}

double ticks_bound(const Value& info, const char16_t* key) {
  const Value v = field_or(info, key);
  if (std::holds_alternative<std::monostate>(v)) return Times::MAX;
  return to_number(v);
}

Value times_snapshot(const Times& t) {
  const std::array<double, 5> nums = t.to_snapshot();
  lfw::Array arr;
  for (double d : nums) arr.push_back(Value(d));
  lfw::Object o;
  o.set(u"nums", Value(std::make_shared<lfw::Array>(arr)));
  return Value(std::make_shared<lfw::Object>(o));
}

void times_read(Times& t, const Value& s) {
  const Array* arr = as_array(field_or(s, u"nums"));
  std::array<double, 5> nums{};
  for (std::size_t i = 0; i < 5; ++i) {
    nums[i] = arr != nullptr && i < arr->size() ? to_number(arr->at(i)) : 0.0;
  }
  t.read_snapshot(nums);
}

}

DrinkInfo::DrinkInfo(const Value& info) {
  _hp_h_ticks.set_range(0.0, ticks_bound(info, u"hp_h_ticks"));
  _hp_h_value = or_default(field_or(info, u"hp_h_value"), Value(0.0));
  _hp_h_total = or_default(field_or(info, u"hp_h_total"), Value(9999999.0));

  _hp_r_ticks.set_range(0.0, ticks_bound(info, u"hp_r_ticks"));
  _hp_r_value = or_default(field_or(info, u"hp_r_value"), Value(0.0));
  _hp_r_total = or_default(field_or(info, u"hp_r_total"), Value(9999999.0));

  _mp_h_ticks.set_range(0.0, ticks_bound(info, u"mp_h_ticks"));
  _mp_h_value = or_default(field_or(info, u"mp_h_value"), Value(0.0));
  _mp_h_total = or_default(field_or(info, u"mp_h_total"), Value(9999999.0));
}

bool DrinkInfo::hp_h_empty() const { return ge(_hp_h, _hp_h_total) || !truthy(_hp_h_value); }

bool DrinkInfo::hp_r_empty() const { return ge(_hp_r, _hp_r_total) || !truthy(_hp_r_value); }

bool DrinkInfo::mp_h_empty() const { return ge(_mp_h, _mp_h_total) || !truthy(_mp_h_value); }

Value DrinkInfo::to_snapshot() const {
  lfw::Object o;
  o.set(u"hp_h_ticks", times_snapshot(_hp_h_ticks));
  o.set(u"hp_h_value", _hp_h_value);
  o.set(u"hp_h_total", _hp_h_total);
  o.set(u"hp_h", _hp_h);
  o.set(u"hp_r_ticks", times_snapshot(_hp_r_ticks));
  o.set(u"hp_r_total", _hp_r_total);
  o.set(u"hp_r_value", _hp_r_value);
  o.set(u"hp_r", _hp_r);
  o.set(u"mp_h_ticks", times_snapshot(_mp_h_ticks));
  o.set(u"mp_h_value", _mp_h_value);
  o.set(u"mp_h_total", _mp_h_total);
  o.set(u"mp_h", _mp_h);
  return Value(std::make_shared<lfw::Object>(o));
}

void DrinkInfo::from_snapshot(const Value& s) {
  times_read(_hp_h_ticks, field_or(s, u"hp_h_ticks"));
  _hp_h_value = field_or(s, u"hp_h_value");
  _hp_h_total = field_or(s, u"hp_h_total");
  _hp_h = field_or(s, u"hp_h");
  times_read(_hp_r_ticks, field_or(s, u"hp_r_ticks"));
  _hp_r_total = field_or(s, u"hp_r_total");
  _hp_r_value = field_or(s, u"hp_r_value");
  _hp_r = field_or(s, u"hp_r");
  times_read(_mp_h_ticks, field_or(s, u"mp_h_ticks"));
  _mp_h_value = field_or(s, u"mp_h_value");
  _mp_h_total = field_or(s, u"mp_h_total");
  _mp_h = field_or(s, u"mp_h");
}

}
