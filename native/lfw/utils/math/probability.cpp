#include "probability.h"

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

double probability(double times, double p) {
  const double x = clamp(p, 0.0, 1.0);
  if (x <= 0.0) return 0.0;
  if (x >= 1.0) return 1.0;
  return 1.0 - pow(1.0 - x, 1.0 / times);
}

}
