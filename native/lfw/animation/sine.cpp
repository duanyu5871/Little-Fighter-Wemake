#include "lfw/animation/sine.h"

#include "lfw/utils/math/base.h"

namespace lfw {

double Sine::method(double v) {
  return (height() * (sin(v * 2 * PI / 1000) + 1) / 2) + bottom();
}

}
