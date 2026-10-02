#pragma once

#include <optional>

namespace lfw {

struct TransformData {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double rotation = 0.0;
  double scale_x = 1.0;
  double scale_y = 1.0;
  double scale_z = 1.0;
};

class Transform {
 public:
  const TransformData& d() const { return _d; }
  bool is_smoothing() const { return _smoothing; }

  double x() const { return _x; }
  double y() const { return _y; }
  double z() const { return _z; }
  double rotation() const { return _rotation; }
  double scale_x() const { return _scale_x; }
  double scale_y() const { return _scale_y; }
  double scale_z() const { return _scale_z; }

  void set_x(double v) { _d.x = _x = v; }
  void set_y(double v) { _d.y = _y = v; }
  void set_z(double v) { _d.z = _z = v; }
  void set_rotation_value(double v) { _d.rotation = _rotation = wrap(v); }
  void set_scale_x(double v) { _d.scale_x = _scale_x = v; }
  void set_scale_y(double v) { _d.scale_y = _scale_y = v; }
  void set_scale_z(double v) { _d.scale_z = _scale_z = v; }

  TransformData snapshot() const;

  Transform& update(std::optional<double> dt = std::nullopt);

  Transform& set_position(std::optional<double> x = std::nullopt,
                          std::optional<double> y = std::nullopt,
                          std::optional<double> z = std::nullopt);

  Transform& set_scale(std::optional<double> x = std::nullopt,
                       std::optional<double> y = std::nullopt,
                       std::optional<double> z = std::nullopt);

  Transform& set_rotation(std::optional<double> rotation = std::nullopt);

  Transform& move_to(std::optional<double> x = std::nullopt,
                     std::optional<double> y = std::nullopt,
                     std::optional<double> z = std::nullopt,
                     std::optional<double> rate = std::nullopt);

  Transform& scale_to(std::optional<double> x = std::nullopt,
                      std::optional<double> y = std::nullopt,
                      std::optional<double> z = std::nullopt,
                      std::optional<double> rate = std::nullopt);

  Transform& rotate_to(std::optional<double> rotation = std::nullopt,
                       std::optional<double> rate = std::nullopt);

  bool is_arrived(std::optional<double> eps = std::nullopt) const;

 private:
  static double wrap(double a);

  double _x = 0.0;
  double _y = 0.0;
  double _z = 0.0;
  double _scale_x = 1.0;
  double _scale_y = 1.0;
  double _scale_z = 1.0;
  double _rotation = 0.0;
  TransformData _d;
  double _rate = 0.1;
  bool _smoothing = false;
};

}
