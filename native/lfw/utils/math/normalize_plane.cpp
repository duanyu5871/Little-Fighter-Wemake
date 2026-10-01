#include "normalize_plane.h"

namespace lfw {
namespace {

Plane g_result{0.0, 0.0, 0.0, 0.0};

}

const Plane& normalize_plane(double a, double b, double c, double d) {
  if (a < 0.0 || (a == 0.0 && b < 0.0) || (a == 0.0 && b == 0.0 && c < 0.0)) {
    a = -a;
    b = -b;
    c = -c;
    d = -d;
  }
  g_result.a = a;
  g_result.b = b;
  g_result.c = c;
  g_result.d = d;
  return g_result;
}

}
