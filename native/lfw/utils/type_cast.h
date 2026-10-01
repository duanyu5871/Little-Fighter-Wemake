#pragma once

#include <optional>

#include "lfw/core/value.h"
#include "lfw/utils/type_check.h"

namespace lfw {

inline std::optional<double> to_num(const Value& v) {
  const double n = to_number(v);
  return is_num(n) ? std::optional<double>(n) : std::nullopt;
}

inline double to_num(const Value& v, double or_value) {
  const double n = to_number(v);
  return is_num(n) ? n : or_value;
}

}

