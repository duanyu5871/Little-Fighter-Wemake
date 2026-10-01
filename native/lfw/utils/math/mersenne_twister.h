#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace lfw {

class MersenneTwister {
 public:
  static constexpr int k_N = 624;
  static constexpr int k_M = 397;

  explicit MersenneTwister(double seed) { reset(seed); }

  void reset(double seed);

  uint32_t next_int();
  double next_float();
  double range(double min, double max);

  std::optional<double> pick(const std::vector<double>& arr);
  std::optional<double> take(std::vector<double>& arr);

  const uint32_t* mt() const { return _mt; }
  uint32_t matrix() const { return _matrix; }
  uint32_t upper_mask() const { return _upper_mask; }
  uint32_t lower_mask() const { return _lower_mask; }
  int index() const { return _index; }
  double seed() const { return _seed; }
  uint64_t times() const { return _times; }

  uint64_t state_hash() const;

 private:
  void twist();

  uint32_t _mt[k_N]{};
  uint32_t _matrix = 0x9908b0dfu;
  uint32_t _upper_mask = 0x80000000u;
  uint32_t _lower_mask = 0x7fffffffu;
  int _index = k_N + 1;
  double _seed = 0.0;
  uint64_t _times = 0;
};

}
