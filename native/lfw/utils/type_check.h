#pragma once

#include <cmath>
#include <variant>

#include "lfw/core/value.h"

namespace lfw {

inline bool is_nan_num(const Value& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr && std::isnan(*d);
}

inline bool is_num(double d) { return !std::isnan(d) && std::isfinite(d); }

inline bool is_num(const Value& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr && is_num(*d);
}

inline bool is_positive(const Value& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr && is_num(*d) && *d > 0;
}

inline bool not_zero_num(const Value& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr && is_num(*d) && *d != 0;
}

inline bool is_int(double d) { return std::isfinite(d) && std::trunc(d) == d; }

inline bool is_int(const Value& v) {
  const double* d = std::get_if<double>(&v);
  return d != nullptr && is_int(*d);
}

}
