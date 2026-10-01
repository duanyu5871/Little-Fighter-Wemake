#pragma once

#include <limits>

#include "lfw/utils/math/normalize_plane.h"

namespace lfw {

const Plane* calc_plane(double x1, double y1, double z1,
                        double x2, double y2, double z2,
                        double x3, double y3, double z3,
                        double eps = std::numeric_limits<double>::epsilon());

}
