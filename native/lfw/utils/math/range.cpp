#include "range.h"

namespace lfw {

std::optional<std::vector<double>> range(double from, double to, double gap) {
  if (gap == 0.0 || (to - from) / gap < 0.0) return std::nullopt;

  std::vector<double> ret;
  ret.push_back(from);
  for (double i = 1.0;; i += 1.0) {
    const double v = from + i * gap;
    if (gap > 0.0 ? v > to : v < to) break;
    ret.push_back(v);
  }
  return ret;
}

}
