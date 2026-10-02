#include "lfw/bot/is_ray_hit.h"

#include <variant>

#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace bot {
namespace {

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

double num_or(const Value& v, double fallback) {
  return is_undefined(v) ? fallback : to_number(v);
}

Value defaulted(const Value& v, const Value& fallback) {
  return is_undefined(v) ? fallback : v;
}

}

Value is_ray_hit(const Value& a, const Value& b, const Value& ray) {
  const Value p0 = field_or(a, u"position");
  const Value p1 = field_or(b, u"position");

  const double x = to_number(field_or(ray, u"x"));
  const double z = to_number(field_or(ray, u"z"));
  const double min_x = num_or(field_or(ray, u"min_x"), 0.0);
  const double max_x = num_or(field_or(ray, u"max_x"), 10000.0);
  const double min_z = num_or(field_or(ray, u"min_z"), 0.0);
  const double max_z = num_or(field_or(ray, u"max_z"), 10000.0);
  const double max_d = num_or(field_or(ray, u"max_d"),
                              defines::num(u"Defines.DAFUALT_QUBE_LENGTH_POW2"));
  const Value reverse = defaulted(field_or(ray, u"reverse"), Value(false));

  const double facing = to_number(field_or(a, u"facing"));
  const double dx = round_float(to_number(field_or(p1, u"x")) - to_number(field_or(p0, u"x")));
  const double dz = round_float(to_number(field_or(p1, u"z")) - to_number(field_or(p0, u"z")));

  if (!between(facing * dx, min_x, max_x)) return reverse;
  if (!between(abs(dz), min_z, max_z)) return reverse;

  const double rx = x * facing;
  const double rz = z;
  const double d_sq = round_float(rx * rx + rz * rz);
  if (d_sq == 0.0) return reverse;

  const double cross = rx * dz - rz * dx;
  const bool hit = round_float((cross * cross) / d_sq) < max_d;
  return Value(truthy(reverse) ? !hit : hit);
}

}
}
