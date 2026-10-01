#pragma once

#include <array>
#include <vector>

namespace lfw {

class Times {
 public:
  static constexpr double MIN = 0.0;
  static constexpr double MAX = 9007199254740991.0;
  static constexpr double LIFES = 9007199254740991.0;

  explicit Times(double min_v = MIN, double max_v = MAX);

  double value() const { return _value; }
  double min() const { return _min; }
  double max() const { return _max; }
  double lifes() const { return _lifes; }
  double remains() const { return _remains; }
  bool is_max() const { return _value >= _max; }
  bool is_min() const { return _value <= _min; }

  void set_min(double v);
  void set_max(double v);
  void set_value(double v);

  Times& reborn();
  Times& set_range(double min_v, double max_v);
  Times& set_lifes(double v = -1.0);
  Times& reset();
  bool add(double d = 1.0);

  void write_nums(std::vector<double>& nums, size_t i) const;
  void read_nums(const std::vector<double>& nums, size_t i);
  std::array<double, 5> to_snapshot() const;
  Times& read_snapshot(const std::array<double, 5>& s);

 private:
  double _value = MIN;
  double _min = MIN;
  double _max = MAX;
  double _lifes = LIFES;
  double _remains = LIFES;
};

}
