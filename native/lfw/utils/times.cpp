#include "times.h"

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {

Times::Times(double min_v, double max_v) { set_range(min_v, max_v); }

void Times::set_min(double v) { _min = lfw::floor(v); }
void Times::set_max(double v) { _max = lfw::floor(v); }
void Times::set_value(double v) { _value = lfw::floor(v); }

Times& Times::reborn() {
  _max = MAX;
  _value = _min = MIN;
  _lifes = _remains = LIFES;
  return *this;
}

Times& Times::set_range(double min_v, double max_v) {
  const double a = lfw::floor(min_v);
  const double b = lfw::floor(max_v);
  _min = lfw::min(a, b);
  _max = lfw::max(a, b);
  _value = a;
  return *this;
}

Times& Times::set_lifes(double v) {
  _lifes = lfw::floor(v);
  _remains = _lifes;
  return *this;
}

Times& Times::reset() {
  _value = _min;
  _remains = _lifes;
  return *this;
}

bool Times::add(double d) {
  if (_remains == 0.0) return false;
  const double v = _value = round_float(_value + d);
  const bool ret = v >= _max;
  if (ret && _remains > 0.0) --_remains;
  if (v >= _max) _value = _min;
  if (v < _min) _value = _max;
  return ret;
}

void Times::write_nums(std::vector<double>& nums, size_t i) const {
  nums[i] = _value;
  nums[i + 1] = _min;
  nums[i + 2] = _max;
  nums[i + 3] = _lifes;
  nums[i + 4] = _remains;
}

void Times::read_nums(const std::vector<double>& nums, size_t i) {
  _value = nums[i];
  _min = nums[i + 1];
  _max = nums[i + 2];
  _lifes = nums[i + 3];
  _remains = nums[i + 4];
}

std::array<double, 5> Times::to_snapshot() const {
  return std::array<double, 5>{_value, _min, _max, _lifes, _remains};
}

Times& Times::read_snapshot(const std::array<double, 5>& s) {
  _value = s[0];
  _min = s[1];
  _max = s[2];
  _lifes = s[3];
  _remains = s[4];
  return *this;
}

}
