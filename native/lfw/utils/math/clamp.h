#pragma once

namespace lfw {

inline double clamp(double value, double lo, double hi) {
  return value < lo ? lo : value > hi ? hi : value;
}

}
