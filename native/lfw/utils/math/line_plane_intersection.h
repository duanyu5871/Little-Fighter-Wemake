#pragma once

#include <limits>

namespace lfw {

struct Vec3 {
  double x;
  double y;
  double z;
};

const Vec3* line_plane_intersection(double a, double b, double c, double d,
                                    double x1, double y1, double z1,
                                    double x2, double y2, double z2,
                                    bool is_direction = false,
                                    bool is_segment = false,
                                    double eps = std::numeric_limits<double>::epsilon());

}
