#include "mersenne_twister.h"

#include <cmath>
#include <cstdint>

#include "js_num.h"
#include "state_hash.h"

namespace lfw {

void MersenneTwister::reset(double seed) {
  _matrix = 0x9908b0dfu;
  _upper_mask = 0x80000000u;
  _lower_mask = 0x7fffffffu;
  _index = k_N + 1;
  _seed = seed;
  _times = 0;

  _mt[0] = js_to_uint32(seed);

  for (int i = 1; i < k_N; ++i) {
    const uint32_t prev = _mt[i - 1];
    const uint32_t s = prev ^ (prev >> 30);

    const uint64_t hi = static_cast<uint64_t>((s & 0xffff0000u) >> 16);
    const uint64_t lo = static_cast<uint64_t>(s & 0x0000ffffu);

    const uint32_t part_hi = static_cast<uint32_t>(hi * 1812433253ull) << 16;
    const uint32_t part_lo = static_cast<uint32_t>(lo * 1812433253ull);

    _mt[i] = part_hi + part_lo + static_cast<uint32_t>(i);
  }
}

void MersenneTwister::twist() {
  for (int i = 0; i < k_N; ++i) {
    const uint32_t x =
        (_mt[i] & _upper_mask) | (_mt[(i + 1) % k_N] & _lower_mask);

    uint32_t xa = x >> 1;
    if ((x & 1u) != 0u) xa ^= _matrix;

    _mt[i] = _mt[(i + k_M) % k_N] ^ xa;
  }
  _index = 0;
}

uint32_t MersenneTwister::next_int() {
  if (_index >= k_N) twist();

  uint32_t y = _mt[_index++];
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680u;
  y ^= (y << 15) & 0xefc60000u;
  y ^= (y >> 18);

  ++_times;
  return y;
}

double MersenneTwister::next_float() {
  return floor_float(static_cast<double>(next_int()) / 4294967296.0);
}

double MersenneTwister::range(double min, double max) {
  if (min == max) return min;
  return std::floor(next_float() * (max - min)) + min;
}

std::optional<double> MersenneTwister::pick(const std::vector<double>& arr) {
  const double index = range(0.0, static_cast<double>(arr.size()));
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  return arr[i];
}

std::optional<double> MersenneTwister::take(std::vector<double>& arr) {
  const double index = range(0.0, static_cast<double>(arr.size()));
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  const double v = arr[i];
  arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(i));
  return v;
}

uint64_t MersenneTwister::state_hash() const {
  StateHash h;
  h.u32(_matrix);
  h.u32(_upper_mask);
  h.u32(_lower_mask);
  h.u32(static_cast<uint32_t>(_index));
  h.f64(_seed);
  h.u64(_times);
  for (int i = 0; i < k_N; ++i) h.u32(_mt[i]);
  return h.value();
}

}
