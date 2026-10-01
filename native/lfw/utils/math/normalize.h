#pragma once

#include "lfw/utils/math/floor_float.h"

namespace lfw {

inline double normalize(double n, double p = 1000.0) {
  n = floor_float(n, p);
  if (n > 0) return 1.0;
  if (n < 0) return -1.0;
  return 0.0;
}

}
