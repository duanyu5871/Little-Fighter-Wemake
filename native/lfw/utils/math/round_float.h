#pragma once

#include <cmath>

#include "lfw/utils/math/base.h"

namespace lfw {

inline double round_float(double n, double multiplier = 1000.0) {
  if (std::isnan(n) || n == 0.0) return n;
  return round(n * multiplier) / multiplier;
}

}
