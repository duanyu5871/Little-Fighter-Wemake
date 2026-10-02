// 已删除的等价变异（不要重加）：
// `next_float` 的除数 4294967296.0 改成 4294967295.0 —— 相对差只有 2.3e-10，而
// `floor_float` 会把结果量化到 1/1000，要让它可观察得满足 int/2^32*1000 落在整数下方
// 2.3e-7 以内，即 int 恰好是 2^29 的倍数（536870912 / 1073741824 / …）。
// 十来个种子里抽一万次也遇不上（单次概率 ~2e-6）⇒ 实际不可观察，改测「除数写成 2^31」。
export default {
  subject: "mersenne_twister",
  mutations: [
    {
      note: "reset：matrix 常量写错一位",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _matrix = 0x9908b0dfu;`,
      to: `  _matrix = 0x9908b0deu;`,
    },
    {
      note: "reset：upper_mask 写错",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _upper_mask = 0x80000000u;`,
      to: `  _upper_mask = 0x40000000u;`,
    },
    {
      note: "reset：lower_mask 写错",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _lower_mask = 0x7fffffffu;`,
      to: `  _lower_mask = 0x3fffffffu;`,
    },
    {
      note: "reset：初始 index 少 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _index = k_N + 1;`,
      to: `  _index = k_N;`,
    },
    {
      note: "reset：不记 seed",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _seed = seed;`,
      to: `  _seed = 0.0;`,
    },
    {
      note: "reset：times 初值写成 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _times = 0;`,
      to: `  _times = 1;`,
    },
    {
      note: "reset：首元素不从种子取",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _mt[0] = js_to_uint32(seed);`,
      to: `  _mt[0] = 0u;`,
    },
    {
      note: "reset：初始化循环的移位量写成 29",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    const uint32_t s = prev ^ (prev >> 30);`,
      to: `    const uint32_t s = prev ^ (prev >> 29);`,
    },
    {
      note: "reset：高 16 位取错",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    const uint64_t hi = static_cast<uint64_t>((s & 0xffff0000u) >> 16);`,
      to: `    const uint64_t hi = static_cast<uint64_t>((s & 0xffff0000u) >> 15);`,
    },
    {
      note: "reset：乘数写错",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    const uint32_t part_hi = static_cast<uint32_t>(hi * 1812433253ull) << 16;`,
      to: `    const uint32_t part_hi = static_cast<uint32_t>(hi * 1812433254ull) << 16;`,
    },
    {
      note: "reset：高半部分少左移一位",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    const uint32_t part_hi = static_cast<uint32_t>(hi * 1812433253ull) << 16;`,
      to: `    const uint32_t part_hi = static_cast<uint32_t>(hi * 1812433253ull) << 15;`,
    },
    {
      note: "reset：低半部分多 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    const uint32_t part_lo = static_cast<uint32_t>(lo * 1812433253ull);`,
      to: `    const uint32_t part_lo = static_cast<uint32_t>(lo * 1812433253ull) + 1u;`,
    },
    {
      note: "reset：不加下标偏移",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    _mt[i] = part_hi + part_lo + static_cast<uint32_t>(i);`,
      to: `    _mt[i] = part_hi + part_lo;`,
    },

    {
      note: "twist：upper/lower 掩码用反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `        (_mt[i] & _upper_mask) | (_mt[(i + 1) % k_N] & _lower_mask);`,
      to: `        (_mt[i] & _lower_mask) | (_mt[(i + 1) % k_N] & _upper_mask);`,
    },
    {
      note: "twist：取下一个状态时模数写成 k_N - 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `        (_mt[i] & _upper_mask) | (_mt[(i + 1) % k_N] & _lower_mask);`,
      to: `        (_mt[i] & _upper_mask) | (_mt[(i + 1) % (k_N - 1)] & _lower_mask);`,
    },
    {
      note: "twist：右移量写成 2",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    uint32_t xa = x >> 1;`,
      to: `    uint32_t xa = x >> 2;`,
    },
    {
      note: "twist：奇数判定取反（偶数才异或 matrix）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    if ((x & 1u) != 0u) xa ^= _matrix;`,
      to: `    if ((x & 1u) == 0u) xa ^= _matrix;`,
    },
    {
      note: "twist：mag 取模写成 k_N - 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    _mt[i] = _mt[(i + k_M) % k_N] ^ xa;`,
      to: `    _mt[i] = _mt[(i + k_M) % (k_N - 1)] ^ xa;`,
    },
    {
      note: "twist：组合用或而不是异或",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    _mt[i] = _mt[(i + k_M) % k_N] ^ xa;`,
      to: `    _mt[i] = _mt[(i + k_M) % k_N] | xa;`,
    },
    {
      note: "twist：结束后 index 写成 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `    _mt[i] = _mt[(i + k_M) % k_N] ^ xa;
  }
  _index = 0;`,
      to: `    _mt[i] = _mt[(i + k_M) % k_N] ^ xa;
  }
  _index = 1;`,
    },

    {
      note: "next_int：提前一个下标就 twist",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (_index >= k_N) twist();`,
      to: `  if (_index >= k_N - 1) twist();`,
    },
    {
      note: "next_int：读取后不推进下标",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  uint32_t y = _mt[_index++];`,
      to: `  uint32_t y = _mt[_index];`,
    },
    {
      note: "next_int：temper 第一步移位写成 10",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y >> 11);`,
      to: `  y ^= (y >> 10);`,
    },
    {
      note: "next_int：temper 第二步左移写成 8",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y << 7) & 0x9d2c5680u;`,
      to: `  y ^= (y << 8) & 0x9d2c5680u;`,
    },
    {
      note: "next_int：temper 第二步掩码第 8 位翻转",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y << 7) & 0x9d2c5680u;`,
      to: `  y ^= (y << 7) & 0x9d2c5780u;`,
    },
    {
      note: "next_int：temper 第三步左移写成 16",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y << 15) & 0xefc60000u;`,
      to: `  y ^= (y << 16) & 0xefc60000u;`,
    },
    {
      note: "next_int：temper 第三步掩码第 15 位翻转",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y << 15) & 0xefc60000u;`,
      to: `  y ^= (y << 15) & 0xefc68000u;`,
    },
    {
      note: "next_int：temper 末步移位写成 17",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  y ^= (y >> 18);`,
      to: `  y ^= (y >> 17);`,
    },
    {
      note: "next_int：times 记成 2",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  ++_times;`,
      to: `  _times += 2;`,
    },

    {
      note: "next_float：除数写成 2^31",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  return floor_float(static_cast<double>(next_int()) / 2147483648.0);`,
    },
    {
      note: "next_float：不做量化",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  return static_cast<double>(next_int()) / 4294967296.0;`,
    },
    {
      note: "next_float：量化倍数写成 10",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  return floor_float(static_cast<double>(next_int()) / 4294967296.0, 10.0);`,
    },

    {
      note: "range：min == max 时返回 max（丢掉 -0）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (min == max) return min;`,
      to: `  if (min == max) return max;`,
    },
    {
      note: "range：min == max 时仍抽取随机数",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (min == max) return min;`,
      to: `  if (false) return min;`,
    },
    {
      note: "range：最后加 min 写成减 min",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor(next_float() * (max - min)) + min;`,
      to: `  return floor(next_float() * (max - min)) - min;`,
    },
    {
      note: "range：区间宽度算反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor(next_float() * (max - min)) + min;`,
      to: `  return floor(next_float() * (min - max)) + min;`,
    },
    {
      note: "range：向下取整改成向上取整",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  return floor(next_float() * (max - min)) + min;`,
      to: `  return ceil(next_float() * (max - min)) + min;`,
    },

    {
      note: "pick：取值上界少 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double index = range(0.0, static_cast<double>(arr.size()));
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  return arr[i];`,
      to: `  const double index = range(0.0, static_cast<double>(arr.size()) - 1.0);
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  return arr[i];`,
    },
    {
      note: "pick：越界时返回 0 而不是 nullopt",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (i >= arr.size()) return std::nullopt;
  return arr[i];`,
      to: `  if (i >= arr.size()) return 0.0;
  return arr[i];`,
    },

    {
      note: "take：取值上界少 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double index = range(0.0, static_cast<double>(arr.size()));
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  const double v = arr[i];`,
      to: `  const double index = range(0.0, static_cast<double>(arr.size()) - 1.0);
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  const double v = arr[i];`,
    },
    {
      note: "take：返回首元素而不是抽中元素",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double v = arr[i];`,
      to: `  const double v = arr[0];`,
    },
    {
      note: "take：删除首元素而不是抽中元素",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(i));`,
      to: `  arr.erase(arr.begin());`,
    },
    {
      note: "take：删除末元素",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(i));`,
      to: `  arr.erase(arr.end() - 1);`,
    },

    {
      note: "state_hash：哈希漏掉最后一个状态槽",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  for (int i = 0; i < k_N; ++i) h.u32(_mt[i]);`,
      to: `  for (int i = 0; i < k_N - 1; ++i) h.u32(_mt[i]);`,
    },
    {
      note: "state_hash：seed 按 u32 而不是 f64",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  h.f64(_seed);`,
      to: `  h.u32(static_cast<uint32_t>(_seed));`,
    },
    {
      note: "state_hash：index 哈希偏移 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  h.u32(static_cast<uint32_t>(_index));`,
      to: `  h.u32(static_cast<uint32_t>(_index) + 1u);`,
    },
    {
      note: "state_hash：times 哈希偏移 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  h.u64(_times);`,
      to: `  h.u64(_times + 1u);`,
    },
    {
      note: "state_hash：matrix 哈希错列",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  h.u32(_matrix);
  h.u32(_upper_mask);`,
      to: `  h.u32(_upper_mask);
  h.u32(_matrix);`,
    },
    {
      note: "state_hash：lower_mask 哈希偏移 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  h.u32(_lower_mask);`,
      to: `  h.u32(_lower_mask ^ 1u);`,
    },
  ],
};
