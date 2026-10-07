#include "lfw/animation/cosine.h"

#include "lfw/utils/math/base.h"

namespace lfw {

double Cosine::method(double v) {
  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();
}

}
