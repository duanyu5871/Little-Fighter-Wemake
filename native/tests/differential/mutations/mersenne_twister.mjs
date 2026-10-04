// 已删除的等价变异（不要重加）：
// `next_float` 的除数 4294967296.0 改成 4294967295.0 —— 相对差只有 2.3e-10，而
// `floor_float` 会把结果量化到 1/1000，要让它可观察得满足 int/2^32*1000 落在整数下方
// 2.3e-7 以内，即 int 恰好是 2^29 的倍数（536870912 / 1073741824 / …）。
// 十来个种子里抽一万次也遇不上（单次概率 ~2e-6）⇒ 实际不可观察，改测「除数写成 2^31」。
// 本主题不可观察（不列变异，原因是逐条的）：
// - `next_int` 里 `log_entry` 与 `++_times` 的先后：条目文本不含 mt 自己的 times，
//   `state` 哈希在两者之后取，任何顺序都同值。
// - `load(info)` / `reset(seed, debuging)` 返回 `*this`：TS 的 `: this` 只用于链式调用，
//   两边都没有链式调用点，harness 也不链式 ⇒ 返回什么都一样。
// - `MersenneTwisterInfo` 的字段默认值：`pure()` 会写满全部字段，harness 也总是以
//   `mt.pure()` 为底再改 ⇒ 默认值不可见。
// - `pure()` 返回的 `mt` 是拷贝还是别名：TS 侧是 `[...this.mt]`（拷贝），C++ 侧是
//   `std::array` 值语义（天然拷贝）⇒ 无法构造出"别名"的 C++ 变异。
// - `mt_cases()` 的"非单例"变异（去掉 `static`）会返回局部对象的引用，MSVC 直接报错，
//   属于非法变异，不列。
// - `log_entry` 的参数按值还是按引用：调用点全是 `{}` 临时量，改成引用直接编译不过。
// - TS `range(min, max, debugging = this.debugging)` 的第三个参数在 TS 里就是**死参数**
//   （函数体只看 `this.debugging`）⇒ 端口不保留该参数，也没有对应变异。

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
      from: `  const double ret = floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  const double ret = floor_float(static_cast<double>(next_int()) / 2147483648.0);`,
    },
    {
      note: "next_float：不做量化",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double ret = floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  const double ret = static_cast<double>(next_int()) / 4294967296.0;`,
    },
    {
      note: "next_float：量化倍数写成 10",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double ret = floor_float(static_cast<double>(next_int()) / 4294967296.0);`,
      to: `  const double ret = floor_float(static_cast<double>(next_int()) / 4294967296.0, 10.0);`,
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
      from: `  const double ret = floor(next_float() * (max - min)) + min;`,
      to: `  const double ret = floor(next_float() * (max - min)) - min;`,
    },
    {
      note: "range：区间宽度算反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double ret = floor(next_float() * (max - min)) + min;`,
      to: `  const double ret = floor(next_float() * (min - max)) + min;`,
    },
    {
      note: "range：向下取整改成向上取整",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double ret = floor(next_float() * (max - min)) + min;`,
      to: `  const double ret = ceil(next_float() * (max - min)) + min;`,
    },

    {
      note: "pick：取值上界少 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const double index = range(0.0, static_cast<double>(arr.size()));
  log_entry(u"pick", {Value(static_cast<double>(arr.size())), Value(index)});
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  return arr[i];`,
      to: `  const double index = range(0.0, static_cast<double>(arr.size()) - 1.0);
  log_entry(u"pick", {Value(static_cast<double>(arr.size())), Value(index)});
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
  log_entry(u"take", {Value(static_cast<double>(arr.size())), Value(index)});
  const uint32_t i = js_to_uint32(index);
  if (i >= arr.size()) return std::nullopt;
  const double v = arr[i];`,
      to: `  const double index = range(0.0, static_cast<double>(arr.size()) - 1.0);
  log_entry(u"take", {Value(static_cast<double>(arr.size())), Value(index)});
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
    {
      note: "reset：忽略第二个参数（debuging）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  debugging = debuging;`,
      to: `  debugging = false;`,
    },
    {
      note: "reset：debugging 恒为 true",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  debugging = debuging;`,
      to: `  debugging = true;`,
    },
    {
      note: "reset：不清空 mark",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  debugging = debuging;\n  mark.clear();`,
      to: `  debugging = debuging;`,
    },
    {
      note: "reset：mark 清成常量",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  mark.clear();`,
      to: `  mark = u\"x\";`,
    },
    {
      note: "log_entry：不看 debugging 就记",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  args.insert(args.begin(), Value(tag));`,
      to: `  args.insert(args.begin(), Value(tag));`,
    },
    {
      note: "log_entry：debugging 判反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  args.insert(args.begin(), Value(tag));`,
      to: `  if (debugging) return;\n  args.insert(args.begin(), Value(tag));`,
    },
    {
      note: "log_entry：tag 放到参数末尾",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  args.insert(args.begin(), Value(tag));`,
      to: `  args.push_back(Value(tag));`,
    },
    {
      note: "log_entry：丢掉 tag",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  args.insert(args.begin(), Value(tag));\n  mt_cases().push(mark, args);`,
      to: `  mt_cases().push(mark, args);`,
    },
    {
      note: "log_entry：不传 mark",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  args.insert(args.begin(), Value(tag));\n  mt_cases().push(mark, args);`,
      to: `  args.insert(args.begin(), Value(tag));\n  mt_cases().push(u\"\", args);`,
    },
    {
      note: "log_entry：写进临时 Cases（不是 mt_cases）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  mt_cases().push(mark, args);\n}\n\nvoid MersenneTwister::log_case`,
      to: `  Cases(u\"mt\").push(mark, args);\n}\n\nvoid MersenneTwister::log_case`,
    },
    {
      note: "log_case：不看 debugging 就记",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  mt_cases().push(mark, args);\n}`,
      to: `  mt_cases().push(mark, args);\n}`,
    },
    {
      note: "log_case：debugging 判反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  mt_cases().push(mark, args);\n}`,
      to: `  if (debugging) return;\n  mt_cases().push(mark, args);\n}`,
    },
    {
      note: "log_case：不传 mark",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  mt_cases().push(mark, args);\n}`,
      to: `  if (!debugging) return;\n  mt_cases().push(u\"\", args);\n}`,
    },
    {
      note: "log_case：丢掉参数",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  if (!debugging) return;\n  mt_cases().push(mark, args);\n}`,
      to: `  if (!debugging) return;\n  mt_cases().push(mark, {});\n}`,
    },
    {
      note: "next_int：标签写成 Int",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"int\", {Value(static_cast<double>(y))});`,
      to: `  log_entry(u\"Int\", {Value(static_cast<double>(y))});`,
    },
    {
      note: "next_int：记录的值加 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"int\", {Value(static_cast<double>(y))});`,
      to: `  log_entry(u\"int\", {Value(static_cast<double>(y + 1u))});`,
    },
    {
      note: "next_int：不记录",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"int\", {Value(static_cast<double>(y))});\n  ++_times;`,
      to: `  ++_times;`,
    },
    {
      note: "next_float：标签写成 Float",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"float\", {Value(ret)});`,
      to: `  log_entry(u\"Float\", {Value(ret)});`,
    },
    {
      note: "next_float：不记录",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"float\", {Value(ret)});\n  return ret;`,
      to: `  return ret;`,
    },
    {
      note: "range：标签写成 Range",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"range\", {Value(min), Value(max), Value(ret)});`,
      to: `  log_entry(u\"Range\", {Value(min), Value(max), Value(ret)});`,
    },
    {
      note: "range：min / max 传反",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"range\", {Value(min), Value(max), Value(ret)});`,
      to: `  log_entry(u\"range\", {Value(max), Value(min), Value(ret)});`,
    },
    {
      note: "range：不记录结果",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"range\", {Value(min), Value(max), Value(ret)});`,
      to: `  log_entry(u\"range\", {Value(min), Value(max)});`,
    },
    {
      note: "pick（double 版）：下标一律记 0",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"pick\", {Value(static_cast<double>(arr.size())), Value(index)});`,
      to: `  log_entry(u\"pick\", {Value(static_cast<double>(arr.size())), Value(0.0)});`,
    },
    {
      note: "pick（double 版）：长度多 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"pick\", {Value(static_cast<double>(arr.size())), Value(index)});`,
      to: `  log_entry(u\"pick\", {Value(static_cast<double>(arr.size()) + 1.0), Value(index)});`,
    },
    {
      note: "pick_value：标签写成 take",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"pick\", {Value(static_cast<double>(arr->size())), Value(index)});`,
      to: `  log_entry(u\"take\", {Value(static_cast<double>(arr->size())), Value(index)});`,
    },
    {
      note: "take（double 版）：标签写成 Take",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"take\", {Value(static_cast<double>(arr.size())), Value(index)});`,
      to: `  log_entry(u\"Take\", {Value(static_cast<double>(arr.size())), Value(index)});`,
    },
    {
      note: "take（double 版）：删除后再记录（长度是删完的）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  log_entry(u\"take\", {Value(static_cast<double>(arr.size())), Value(index)});\n  const uint32_t i = js_to_uint32(index);\n  if (i >= arr.size()) return std::nullopt;\n  const double v = arr[i];\n  arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(i));\n  return v;`,
      to: `  const uint32_t i = js_to_uint32(index);\n  if (i >= arr.size()) return std::nullopt;\n  const double v = arr[i];\n  arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(i));\n  log_entry(u\"take\", {Value(static_cast<double>(arr.size())), Value(index)});\n  return v;`,
    },
    {
      note: "take_value：删除下标写死 0",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  arr->remove_at(i);`,
      to: `  arr->remove_at(0);`,
    },
    {
      note: "take_value：删除下标 i + 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  arr->remove_at(i);`,
      to: `  arr->remove_at(static_cast<size_t>(i) + 1);`,
    },
    {
      note: "take_value：不删除",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const Value v = arr->at(i);\n  arr->remove_at(i);`,
      to: `  const Value v = arr->at(i);`,
    },
    {
      note: "take_value：返回删除后的同一位置（下一项）",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  const Value v = arr->at(i);\n  arr->remove_at(i);\n  return v;`,
      to: `  arr->remove_at(i);\n  return arr->at(i);`,
    },
    {
      note: "pure：matrix 取自 upper_mask",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.matrix = _matrix;`,
      to: `  info.matrix = _upper_mask;`,
    },
    {
      note: "pure：upper_mask 取自 lower_mask",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.upper_mask = _upper_mask;`,
      to: `  info.upper_mask = _lower_mask;`,
    },
    {
      note: "pure：lower_mask 取自 matrix",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.lower_mask = _lower_mask;`,
      to: `  info.lower_mask = _matrix;`,
    },
    {
      note: "pure：index 偏移 1",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.index = _index;`,
      to: `  info.index = _index + 1;`,
    },
    {
      note: "pure：seed 不拷贝",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.seed = _seed;`,
      to: `  info.seed = 0.0;`,
    },
    {
      note: "pure：times 不拷贝",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.times = _times;`,
      to: `  info.times = 0;`,
    },
    {
      note: "pure：mark 不拷贝",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  info.mark = mark;`,
      to: `  info.mark.clear();`,
    },
    {
      note: "pure：mt 只拷首尾两格",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  std::copy(_mt.begin(), _mt.end(), info.mt.begin());`,
      to: `  info.mt.front() = _mt.front();\n  info.mt.back() = _mt.back();`,
    },
    {
      note: "pure：mt 整体不拷贝",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  std::copy(_mt.begin(), _mt.end(), info.mt.begin());`,
      to: `  (void)0;`,
    },
    {
      note: "load：matrix / upper_mask 换位",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _matrix = info.matrix;\n  _upper_mask = info.upper_mask;`,
      to: `  _matrix = info.upper_mask;\n  _upper_mask = info.matrix;`,
    },
    {
      note: "load：不理 lower_mask",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _lower_mask = info.lower_mask;`,
      to: `  (void)info.lower_mask;`,
    },
    {
      note: "load：不理 mt",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _mt = info.mt;`,
      to: `  (void)info.mt;`,
    },
    {
      note: "load：不理 index",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _index = info.index;`,
      to: `  (void)info.index;`,
    },
    {
      note: "load：不理 seed",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _seed = info.seed;`,
      to: `  (void)info.seed;`,
    },
    {
      note: "load：不理 times",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  _times = info.times;`,
      to: `  (void)info.times;`,
    },
    {
      note: "load：不理 mark",
      file: "native/lfw/utils/math/mersenne_twister.cpp",
      from: `  mark = info.mark;`,
      to: `  mark.clear();`,
    },
    {
      note: "Cases：分隔符换成逗号",
      file: "native/lfw/cases.h",
      from: `  std::u16string _separator = u\"\\uffe5\";`,
      to: `  std::u16string _separator = u\",\";`,
    },
    {
      note: "Cases：分隔符换一个码位",
      file: "native/lfw/cases.h",
      from: `  std::u16string _separator = u\"\\uffe5\";`,
      to: `  std::u16string _separator = u\"\\uffe4\";`,
    },
    {
      note: "Cases：不保存传入的 name",
      file: "native/lfw/cases.h",
      from: `  explicit Cases(std::u16string name) : _name(std::move(name)) {}`,
      to: `  explicit Cases(std::u16string name) : _name(u\"\") {}`,
    },
    {
      note: "mt_cases：名字写成 MT",
      file: "native/lfw/cases.cpp",
      from: `  static Cases instance(u\"mt\");`,
      to: `  static Cases instance(u\"MT\");`,
    },
    {
      note: "Cases::reset：不清 times（编号接着数）",
      file: "native/lfw/cases.cpp",
      from: `void Cases::reset() {\n  _times = 0;\n  _cases.clear();`,
      to: `void Cases::reset() {\n  _cases.clear();`,
    },
    {
      note: "Cases::reset：不清 cases",
      file: "native/lfw/cases.cpp",
      from: `  _times = 0;\n  _cases.clear();\n}`,
      to: `  _times = 0;\n}`,
    },
    {
      note: "Cases::reset：什么都不做",
      file: "native/lfw/cases.cpp",
      from: `void Cases::reset() {\n  _times = 0;\n  _cases.clear();\n}`,
      to: `void Cases::reset() {\n}`,
    },
    {
      note: "Cases::push：编号不自增（恒为 1）",
      file: "native/lfw/cases.cpp",
      from: `  text += number_to_string(static_cast<double>(++_times));`,
      to: `  _times = 1;\n  text += number_to_string(static_cast<double>(_times));`,
    },
    {
      note: "Cases::push：编号用 times - 1",
      file: "native/lfw/cases.cpp",
      from: `  text += number_to_string(static_cast<double>(++_times));`,
      to: `  text += number_to_string(static_cast<double>(++_times) - 1);`,
    },
    {
      note: "Cases::push：前括号写成方括号",
      file: "native/lfw/cases.cpp",
      from: `  text.push_back(u'(');`,
      to: `  text.push_back(u'[');`,
    },
    {
      note: "Cases::push：后括号写成方括号",
      file: "native/lfw/cases.cpp",
      from: `  text.push_back(u')');`,
      to: `  text.push_back(u']');`,
    },
    {
      note: "Cases::push：参数区前缀写成冒号",
      file: "native/lfw/cases.cpp",
      from: `    text += u\":[\";\n    text += array_join(arr);`,
      to: `    text += u\":\";\n    text += array_join(arr);`,
    },
    {
      note: "Cases::push：无参数也带 :[]",
      file: "native/lfw/cases.cpp",
      from: `  if (!args.empty()) {`,
      to: `  if (true) {`,
    },
    {
      note: "Cases::push：只拼第一个参数",
      file: "native/lfw/cases.cpp",
      from: `    const Array arr(args);`,
      to: `    const Array arr({args.front()});`,
    },
    {
      note: "Cases::push：参数区尾巴写成圆括号",
      file: "native/lfw/cases.cpp",
      from: `    text.push_back(u']');`,
      to: `    text.push_back(u')');`,
    },
    {
      note: "Cases::submit：分隔符常数替换",
      file: "native/lfw/cases.cpp",
      from: `    if (i != 0) out += _separator;`,
      to: `    if (i != 0) out += u\",\";`,
    },
    {
      note: "Cases::submit：首条前也多一个分隔符",
      file: "native/lfw/cases.cpp",
      from: `    if (i != 0) out += _separator;`,
      to: `    out += _separator;`,
    },
    {
      note: "Cases::submit：倒序拼接",
      file: "native/lfw/cases.cpp",
      from: `    out += _cases[i];`,
      to: `    out += _cases[_cases.size() - 1 - i];`,
    },
    {
      note: "Cases::submit：不清 cases",
      file: "native/lfw/cases.cpp",
      from: `  _cases.clear();\n  return out;`,
      to: `  return out;`,
    },
  ],
};
