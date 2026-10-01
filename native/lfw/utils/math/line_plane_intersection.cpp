#include "line_plane_intersection.h"

#include "lfw/utils/math/base.h"

namespace lfw {
namespace {

Vec3 g_result{0.0, 0.0, 0.0};

}

const Vec3* line_plane_intersection(double a, double b, double c, double d,
                                    double x1, double y1, double z1,
                                    double x2, double y2, double z2,
                                    bool is_direction,
                                    bool is_segment,
                                    double eps) {
  double vx;
  double vy;
  double vz;
  if (is_direction) {
    vx = x2;
    vy = y2;
    vz = z2;
  } else {
    vx = x2 - x1;
    vy = y2 - y1;
    vz = z2 - z1;
  }

  const double denom = a * vx + b * vy + c * vz;
  if (abs(denom) < eps) return nullptr;

  const double t = -(a * x1 + b * y1 + c * z1 + d) / denom;

  if (is_segment && (t < -eps || t > 1.0 + eps)) return nullptr;

  g_result.x = x1 + t * vx;
  g_result.y = y1 + t * vy;
  g_result.z = z1 + t * vz;
  return &g_result;
}

}
