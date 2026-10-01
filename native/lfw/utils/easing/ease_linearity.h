#pragma once

namespace lfw {

inline double ease_linearity(double factor, double from = 0.0, double to = 1.0) {
  return from + (to - from) * factor;
}

inline double ease_linearity_backward(double v, double from = 0.0, double to = 1.0) {
  return (v - from) / (to - from);
}

}
