#include "project_to_line.h"

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {

std::optional<std::array<double, 2>> project_to_line(double x, double y, double m, double n) {
  const double d = round_float(pow(m, 2.0) + pow(n, 2.0));
  if (d == 0.0) return std::nullopt;

  const double t = ((x - 0.0) * m + (y - 0.0) * n) / d;
  return std::array<double, 2>{round_float(t * m), round_float(t * n)};
}

}
