#include "transform.h"

#include <cmath>

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {

double Transform::wrap(double a) {
  double r = std::fmod(a, PI * 2.0);
  if (r > PI) r -= PI * 2.0;
  else if (r < -PI) r += PI * 2.0;
  return r;
}

TransformData Transform::snapshot() const {
  TransformData out;
  out.x = _x;
  out.y = _y;
  out.z = _z;
  out.rotation = _rotation;
  out.scale_x = _scale_x;
  out.scale_y = _scale_y;
  out.scale_z = _scale_z;
  return out;
}

Transform& Transform::update(std::optional<double> dt) {
  const double step = dt.value_or(1.0);
  if (!_smoothing) return *this;

  const TransformData& target = _d;
  const double k = 1.0 - pow(1.0 - _rate, step);
  _x = round_float(_x + (target.x - _x) * k, 100.0);
  _y = round_float(_y + (target.y - _y) * k, 100.0);
  _z = round_float(_z + (target.z - _z) * k, 100.0);
  _rotation = wrap(_rotation + wrap(target.rotation - _rotation) * k);
  _scale_x = round_float(_scale_x + (target.scale_x - _scale_x) * k, 10000.0);
  _scale_y = round_float(_scale_y + (target.scale_y - _scale_y) * k, 10000.0);
  _scale_z = round_float(_scale_z + (target.scale_z - _scale_z) * k, 10000.0);

  if (is_arrived()) {
    _x = target.x;
    _y = target.y;
    _z = target.z;
    _rotation = target.rotation;
    _scale_x = target.scale_x;
    _scale_y = target.scale_y;
    _scale_z = target.scale_z;
    _smoothing = false;
  }
  return *this;
}

Transform& Transform::set_position(std::optional<double> x, std::optional<double> y,
                                   std::optional<double> z) {
  _d.x = _x = x.value_or(_x);
  _d.y = _y = y.value_or(_y);
  _d.z = _z = z.value_or(_z);
  _smoothing = false;
  return *this;
}

Transform& Transform::set_scale(std::optional<double> x, std::optional<double> y,
                                std::optional<double> z) {
  _d.scale_x = _scale_x = x.value_or(_scale_x);
  _d.scale_y = _scale_y = y.value_or(_scale_y);
  _d.scale_z = _scale_z = z.value_or(_scale_z);
  _smoothing = false;
  return *this;
}

Transform& Transform::set_rotation(std::optional<double> rotation) {
  _d.rotation = _rotation = wrap(rotation.value_or(_rotation));
  _smoothing = false;
  return *this;
}

Transform& Transform::move_to(std::optional<double> x, std::optional<double> y,
                              std::optional<double> z, std::optional<double> rate) {
  _d.x = x.value_or(_x);
  _d.y = y.value_or(_y);
  _d.z = z.value_or(_z);
  _rate = rate.value_or(0.1);
  _smoothing = true;
  return *this;
}

Transform& Transform::scale_to(std::optional<double> x, std::optional<double> y,
                               std::optional<double> z, std::optional<double> rate) {
  _d.scale_x = x.value_or(_scale_x);
  _d.scale_y = y.value_or(_scale_y);
  _d.scale_z = z.value_or(_scale_z);
  _rate = rate.value_or(0.1);
  _smoothing = true;
  return *this;
}

Transform& Transform::rotate_to(std::optional<double> rotation, std::optional<double> rate) {
  _d.rotation = wrap(rotation.value_or(_rotation));
  _rate = rate.value_or(0.1);
  _smoothing = true;
  return *this;
}

bool Transform::is_arrived(std::optional<double> eps) const {
  const double e = eps.value_or(0.01);
  return std::abs(_x - _d.x) < e && std::abs(_y - _d.y) < e && std::abs(_z - _d.z) < e &&
         std::abs(wrap(_rotation - _d.rotation)) < e && std::abs(_scale_x - _d.scale_x) < e &&
         std::abs(_scale_y - _d.scale_y) < e && std::abs(_scale_z - _d.scale_z) < e;
}

}
