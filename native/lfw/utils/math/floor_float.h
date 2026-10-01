#pragma once

#include "lfw/utils/math/base.h"

namespace lfw {

inline double floor_float(double n, double multiplier = 1000.0) {
  return floor(n * multiplier) / multiplier;
}

}
