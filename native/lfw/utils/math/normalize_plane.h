#pragma once

namespace lfw {

struct Plane {
  double a;
  double b;
  double c;
  double d;
};

const Plane& normalize_plane(double a, double b, double c, double d);

}
