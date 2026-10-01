#pragma once

#include "lfw/utils/math/round_float.h"

namespace lfw {

inline double clamp_add(double value, double offset, double lo, double hi) {
  value = round_float(value + offset);
  return value < lo ? lo : value > hi ? hi : value;
}

}
