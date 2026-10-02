#include "lfw/controller/key_status.h"

#include <cmath>
#include <memory>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace controller {
namespace {

bool truthy_num(double n) { return n != 0.0 && !std::isnan(n); }

}

KeyStatus::KeyStatus(const Value& key) : _key(key) {}

double KeyStatus::use() {
  _used = 1;
  return _d_time;
}

bool KeyStatus::is_start(double time) const {
  return truthy_num(_d_time) && _d_time == time;
}

bool KeyStatus::is_hit(double time, double key_hit_duration) const {
  if (!truthy_num(_d_time)) return false;
  const double dt = time - _d_time;
  return dt < key_hit_duration;
}

bool KeyStatus::is_hld(double time, double key_hit_duration) const {
  return !is_hit(time, key_hit_duration) && _d_time > _u_time;
}

bool KeyStatus::is_end() const { return _d_time <= _u_time; }

void KeyStatus::hit(const Value& t, double time) {
  _d_time = std::holds_alternative<std::monostate>(t) ? time : to_number(t);
  _used = 0;
}

void KeyStatus::end(double time) { _u_time = time; }

void KeyStatus::reset() {
  _d_time = 0;
  _u_time = 0;
  _used = 0;
}

Value KeyStatus::to_snapshot() const {
  Array a;
  a.push_back(Value(_d_time));
  a.push_back(Value(_u_time));
  a.push_back(Value(_used));
  return Value(std::make_shared<Array>(a));
}

void KeyStatus::from_snapshot(const Value& s) {
  const Array* a = as_array(s);
  _d_time = a != nullptr && a->size() > 0 ? to_number(a->at(0)) : 0.0;
  _u_time = a != nullptr && a->size() > 1 ? to_number(a->at(1)) : 0.0;
  const Value used = a != nullptr && a->size() > 2 ? a->at(2) : Value();
  _used = truthy(used) ? 1.0 : 0.0;
}

}
}
