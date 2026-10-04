#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

constexpr int kMtN = 624;
constexpr int kMtM = 397;

// `IMersenneTwisterInfo`（`MersenneTwister.ts`）。
struct MersenneTwisterInfo {
  uint32_t matrix = 0;
  uint32_t upper_mask = 0;
  uint32_t lower_mask = 0;
  std::array<uint32_t, kMtN> mt{};
  int index = 0;
  double seed = 0.0;
  uint64_t times = 0;
  std::u16string mark;
};

class MersenneTwister {
 public:
  static constexpr int k_N = kMtN;
  static constexpr int k_M = kMtM;

  explicit MersenneTwister(double seed) { reset(seed); }

  // TS 的两个公开字段（调试用）。
  std::u16string mark;
  bool debugging = false;

  // `reset(seed, debuging = false): this`
  MersenneTwister& reset(double seed, bool debuging = false);

  MersenneTwisterInfo pure() const;
  MersenneTwister& load(const MersenneTwisterInfo& info);

  uint32_t next_int();
  double next_float();
  double range(double min, double max);

  std::optional<double> pick(const std::vector<double>& arr);
  std::optional<double> take(std::vector<double>& arr);
  // `Mt.pick<T>(a)` / `Mt.take<T>(a)`：`if (!a) return void 0; if (!Array.isArray(a)) return a;`
  // 数组分支抽一个下标（`range(0, a.length)`，一次抽取），`take` 还会 `splice` 掉它。
  Value pick_value(const Value& a);
  Value take_value(Value& a);

  // `case(...any)`：只在 `debugging` 时把参数追加进 `mt_cases`。
  void log_case(const std::vector<Value>& args);

  const uint32_t* mt() const { return _mt.data(); }
  uint32_t matrix() const { return _matrix; }
  uint32_t upper_mask() const { return _upper_mask; }
  uint32_t lower_mask() const { return _lower_mask; }
  int index() const { return _index; }
  double seed() const { return _seed; }
  uint64_t times() const { return _times; }

  uint64_t state_hash() const;

 private:
  void twist();
  // `int`/`float`/`range`/`pick`/`take` 里的 `if (this.debugging) mt_cases.push(this.mark, …)`。
  void log_entry(const std::u16string& tag, std::vector<Value> args);

  std::array<uint32_t, kMtN> _mt{};
  uint32_t _matrix = 0x9908b0dfu;
  uint32_t _upper_mask = 0x80000000u;
  uint32_t _lower_mask = 0x7fffffffu;
  int _index = k_N + 1;
  double _seed = 0.0;
  uint64_t _times = 0;
};

}
