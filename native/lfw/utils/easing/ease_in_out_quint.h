#pragma once

#include "lfw/utils/math/base.h"

namespace lfw {

inline double ease_in_out_quint(double factor, double from = 0.0, double to = 1.0) {
  const double ratio =
      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 - pow(-2.0 * factor + 2.0, 5.0) / 2.0;
  return from + ratio * (to - from);
}

inline double ease_in_out_quint_backward(double v, double from = 0.0, double to = 1.0) {
  const double lo = min(from, to);
  const double hi = max(from, to);
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  const double ratio = (v - from) / (to - from);
  if (ratio < 0.5) {
    return pow(ratio / 16.0, 0.2);
  }
  return 1.0 - pow(2.0 * (1.0 - ratio), 0.2) / 2.0;
}

}
