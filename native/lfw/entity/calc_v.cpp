#include "lfw/entity/calc_v.h"

#include <cmath>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace entity {
namespace {

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

double param_or(const Value& v, double fallback) {
  return is_undefined(v) ? fallback : to_number(v);
}

bool falsy_num(double n) { return n == 0.0 || std::isnan(n); }

double soft_target(double current, double target) {
  if (current < target && target > 0) return target;
  if (current > target && target < 0) return target;
  return current;
}

}

double calc_v(double current, double value, const Value& mode, const Value& acc_value,
              const Value& direction_value) {
  const double acc = param_or(acc_value, 0.0);
  const double direction = param_or(direction_value, 1.0);
  const double* raw = std::get_if<double>(&mode);
  const double m = raw != nullptr ? *raw : -1.0;

  if (m == static_cast<double>(SpeedMode::Fixed)) return value;
  if (m == static_cast<double>(SpeedMode::Extra)) return current;
  if (m == static_cast<double>(SpeedMode::FixedAcc)) return current + value;
  if (m == static_cast<double>(SpeedMode::Acc)) return current + value * direction;
  if (m == static_cast<double>(SpeedMode::FixedLf2)) return soft_target(current, value);
  if (m == static_cast<double>(SpeedMode::AccTo)) {
    const double target = value * direction;
    const double scaled_acc = acc * direction;
    if (falsy_num(scaled_acc)) return current;
    if (current >= target && scaled_acc > 0) return current;
    if (current <= target && scaled_acc < 0) return current;
    return current + scaled_acc;
  }
  if (m == static_cast<double>(SpeedMode::FixedAccTo)) {
    const double target = value;
    if (falsy_num(acc)) return current;
    if (current >= target && acc > 0) return current;
    if (current <= target && acc < 0) return current;
    return current + acc;
  }
  return soft_target(current, value * direction);
}

}
}
