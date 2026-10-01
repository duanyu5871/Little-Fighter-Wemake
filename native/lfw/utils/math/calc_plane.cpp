#include "calc_plane.h"

#include "lfw/utils/math/base.h"

namespace lfw {
namespace {

Plane g_result{0.0, 0.0, 0.0, 0.0};

}

const Plane* calc_plane(double x1, double y1, double z1,
                        double x2, double y2, double z2,
                        double x3, double y3, double z3,
                        double eps) {
  const double v1_x = x2 - x1;
  const double v1_y = y2 - y1;
  const double v1_z = z2 - z1;
  const double v2_x = x3 - x1;
  const double v2_y = y3 - y1;
  const double v2_z = z3 - z1;

  const double a = v1_y * v2_z - v1_z * v2_y;
  const double b = v1_z * v2_x - v1_x * v2_z;
  const double c = v1_x * v2_y - v1_y * v2_x;

  if (abs(a) < eps && abs(b) < eps && abs(c) < eps) return nullptr;

  g_result.a = a;
  g_result.b = b;
  g_result.c = c;
  g_result.d = -a * x1 - b * y1 - c * z1;
  return &g_result;
}

}
