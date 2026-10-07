#include "lfw/animation/tangent.h"

#include "lfw/utils/math/base.h"

namespace lfw {

double Tangent::method(double v) { return tan(v * 2 * PI / 1000); }

}
