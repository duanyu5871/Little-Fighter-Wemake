#pragma once

#include "lfw/utils/math/base.h"

namespace lfw {

inline double ease_in_out_sine(double factor, double from = 0.0, double to = 1.0) {
  return from - ((to - from) * (cos(PI * factor) - 1.0)) / 2.0;
}

inline double ease_in_out_sine_backward(double v, double from = 0.0, double to = 1.0) {
  const double lo = min(from, to);
  const double hi = max(from, to);
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return acos((2.0 * (from - v)) / (to - from) + 1.0) / PI;
}

}
