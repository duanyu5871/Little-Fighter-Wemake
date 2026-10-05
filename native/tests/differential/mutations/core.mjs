/**
 * core 变异规格
 *
 * 覆盖 subject: core
 *   core/js_num.cpp   —— js_round / js_floor / js_ceil / js_abs / js_to_uint32 / js_to_int32 / f64_bits
 *   core/js_string.cpp —— is_str_white_space / digit_value / parse_radix / scan_decimal /
 *                         string_to_number / shortest_digits / number_to_string /
 *                         number_to_fixed_1
 *
 * 用例（**文件行数**）：js_num 116 行、to_number 197 行、number_to_string 98 行、
 *       number_to_string_fuzz 10558 行、to_fixed 487 行（其中 400 条是固定种子的随机位模式）。
 *
 * 已删除 / 未注入的变异（不算覆盖缺口，理由记在此处以免以后重复尝试）：
 *  0. number_to_fixed_1 的非零下界 `if (k <= 58)`：|x| ≥ 0.05 时 k ≤ 57，|x| < 0.05 时
 *     n 本来就该是 0 ⇒ 上界取 57 或 58 **等价**（列的是收到 56 的那条，因为 k = 57 的取值
 *     确实存在，如 0.06 与 0.0625）。
 *  0b. number_to_fixed_1 的 `const bool neg = v < 0;` 改成 `v <= 0` —— **按构造等价，已撤出**：
 *     走到 `if (neg) out.push_back(u'-');` 时必有 `a > 0`（`a == 0` 那支提前 return 了），
 *     而 `a > 0` 时 `v <= 0` 与 `v < 0` 同真同假。`-0` 的「没有负号」是**那条提前 return**
 *     给的（它写死了 "0.0"），不是比较符给的 ⇒ 这条永远杀不掉（首轮 127/128 的唯一存活）。
 *  1. js_num.cpp `js_round` 首行 `std::isnan(x)` 与 `std::isinf(x)` 两个条件 —— 去掉它们后
 *     走的是同一个 `std::floor(x + 0.5)` 路径：NaN ⇒ floor(NaN)=NaN、NaN-NaN=NaN、`NaN > 0.5` 假 ⇒ 返回 NaN；
 *     inf ⇒ floor(inf)=inf、inf-inf=NaN ⇒ 返回 inf。与早退分支同值 ⇒ 恒等（只有 `x == 0.0`
 *     那一小段是可观测的：去掉后 `-0` 会变成 `+0`，已被 `round -0` 杀住）。
 *  2. js_num.cpp `js_to_uint32` 的 `!std::isfinite(x)` 与 `x == 0.0` —— 去掉 `isfinite` 会让
 *     inf/NaN 走到 `static_cast<uint32_t>(NaN)`（UB）；去掉 `x == 0.0` 对 `±0` 结果仍是 0（恒等）。
 *  3. js_num.cpp `if (m < 0.0) m += 4294967296.0;` 改成 `<= 0.0` / `> 0.0` 或整行删掉 ——
 *     `m == 0` 时会让 `m` 变成 4294967296.0、`m < 0` 时会让负数直接进 `static_cast<uint32_t>`
 *     ⇒ 都是 UB。
 *  4. js_num.cpp `js_to_int32` 的 `u < 0x80000000u ? (int32_t)u : (int32_t)((int64_t)u - 4294967296ll)`
 *     整段换成只走某一支 —— uint32→int32 / int64→int32 的窄化在 MSVC 上都是"截低 32 位"，
 *     两支结果恒等（这条分支只是把实现定义行为写成显式定义）⇒ 打不出差异。
 *  5. js_num.cpp `f64_bits` / `f64_from_bits` —— `std::bit_cast` 没有可注入的"漂移"。
 *  6. js_string.cpp `parse_radix` 的 `if (shifted > 1100) return infinity;` —— 走到这行时数值
 *     至少是 2^1101，`std::ldexp` 无论如何都返回 inf ⇒ 与不过早退完全同值（它是快速路径，
 *     不是判据）；同理 `if (shifted == 0)`、`if (drop <= 0)`（`shifted > 0` 蕴含 `acc` 占满 64 位、
 *     即 lead == 63 ⇒ drop == 11 > 0，不可达）。
 *  7. js_string.cpp `scan_decimal` 的 `n - i == 8` 改成 `>= 8` 以外 —— 改成 `n - i == 7` 之类
 *     因为比较循环拿 `s[i+k]` 与 `"Infinity"[k]` 逐字符比，长度不对时必然 different ⇒ 恒等；
 *     `for (k = 0; k < 8; ++k)` 改成 `< 7` 也恒等（长度已由 `n - i == 8` 保证）。
 *  8. js_string.cpp `scan_decimal` 的 `e < 1000000` 封顶 —— 它是**有符号溢出保护**：去掉后
 *     19 位以上的指数会让 `int64_t e` 溢出（UB/UB 边界的实现定义行为）。`exp` 下游只用到
 *     `e10` 的符号，而能让符号翻面的指数远小于会让 e 溢出的位数 ⇒ 不打。
 *  9. js_string.cpp `scan_decimal` 里 `int_digits` / `sig_index` 的"少记一位"类改动 ——
 *     `e10 = int_digits - 1 - first_sig + exp` 只在 `from_chars` 判 out_of_range 时才被用，
 *     而那时 `|e10| >= 309`，个位数级的偏移翻不了符号 ⇒ 恒等（能翻面的是 first_sig 本身，
 *     见下面被杀的 c9/c10 与新增的两条长样本）。
 * 10. js_string.cpp `string_to_number` 的 `if (r.ec == std::errc::result_out_of_range)` 里
 *     把 `e10 >= 0` 改成 `e10 > 0` —— 想构造 `e10 == 0` 又要 out_of_range 的输入不存在
 *     （`e10 == 0` 意味着量级约 1e0）⇒ 不可达。
 * 11. js_string.cpp `number_to_string` 的 `const bool neg = v < 0;` 改成 `<= 0` —— `0`/`-0`
 *     在更前面的 `if (v == 0.0) return u"0";` 已经返回 ⇒ 这条分支到不了。
 * 12. `number_to_string` 第 4 支是唯一的"指数形态"，其中 `static_cast<int>(r.ptr - buf)` 之类的
 *     指针算术改动不产生可观测漂移（`shortest_digits` 的返回值只通过 `d`/`exp10` 进入输出）。
 *
 * 首轮跑出的 3 个幸存者（已删，逐条证明）：
 * 13. js_num.cpp `js_to_int32` 把 `u < 0x80000000u` 改成 `u < 0x7fffffffu` —— 只有
 *     `u == 0x7fffffff` 会从正支掉到负支，而 `(int32_t)(2147483647 - 4294967296)` 在 MSVC 上
 *     是"截低 32 位" ⇒ 仍得 2147483647 ⇒ 与正支同值（与第 4 条同源：两支恒等）。
 * 14. `parse_radix` 的 `acc >> (64 - log2base)` 改成 `>> (63 - log2base)` —— 它只改变"什么时候
 *     把满位半字节移出去"：多移一次等价于 `acc' = acc >> 4`、`shifted' = shifted + 4`，
 *     而 `drop' + shifted' = drop + shifted`（指数不变）、`keep' = keep`（尾数不变），
 *     移到窗口外的低位变成 `sticky` —— 正是舍入判决需要的全部信息 ⇒ 判决不变 ⇒ 等价。
 *     已在 20 万条 68~160 位随机 hex 输入上验证零差异。
 * 15. `number_to_string` 的 `const double a = neg ? -v : v;` 改成 `neg ? v : -v` ——
 *     `shortest_digits` 开头就把 `to_chars` 输出里的 `'-'` 跳掉（它只取数字与指数），
 *     符号由后面的 `if (neg) out.push_back(u'-')` 统一给出 ⇒ 传原值还是传相反数完全同果。
 */
export default {
  subject: "core",
  mutations: [
    // ---- js_num.cpp: js_round 的零/半/大整数分界 ----
    {
      note: "js_round: 正半开区间上界改闭",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (x > 0.0 && x < 0.5) return 0.0;",
      to: "  if (x > 0.0 && x <= 0.5) return 0.0;",
    },
    {
      note: "js_round: 正半开区间上界改 1",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (x > 0.0 && x < 0.5) return 0.0;",
      to: "  if (x > 0.0 && x < 1.0) return 0.0;",
    },
    {
      note: "js_round: 负半开区间下界改开",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (x < 0.0 && x >= -0.5) return -0.0;",
      to: "  if (x < 0.0 && x > -0.5) return -0.0;",
    },
    {
      note: "js_round: 负半开区间下界改 -1",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (x < 0.0 && x >= -0.5) return -0.0;",
      to: "  if (x < 0.0 && x >= -1.0) return -0.0;",
    },
    {
      note: "js_round: 丢掉 -0 特判",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (std::isnan(x) || std::isinf(x) || x == 0.0) return x;",
      to: "  if (std::isnan(x) || std::isinf(x)) return x;",
    },
    {
      note: "js_round: 主公式换成 ceil(x - 0.5)",
      file: "native/lfw/core/js_num.cpp",
      from: "  double r = std::floor(x + 0.5);",
      to: "  double r = std::ceil(x - 0.5);",
    },
    {
      note: "js_round: 大整数修正方向取反",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (r - x > 0.5) r -= 1.0;",
      to: "  if (r - x > 0.5) r += 1.0;",
    },
    {
      note: "js_round: 修正阈值改闭",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (r - x > 0.5) r -= 1.0;",
      to: "  if (r - x >= 0.5) r -= 1.0;",
    },
    {
      note: "js_round: 修正条件符号写反",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (r - x > 0.5) r -= 1.0;",
      to: "  if (x - r > 0.5) r -= 1.0;",
    },

    // ---- js_num.cpp: floor / ceil / abs ----
    {
      note: "js_floor: 换成 trunc",
      file: "native/lfw/core/js_num.cpp",
      from: "double js_floor(double x) { return std::floor(x); }",
      to: "double js_floor(double x) { return std::trunc(x); }",
    },
    {
      note: "js_floor: 换成 ceil",
      file: "native/lfw/core/js_num.cpp",
      from: "double js_floor(double x) { return std::floor(x); }",
      to: "double js_floor(double x) { return std::ceil(x); }",
    },
    {
      note: "js_ceil: 换成 trunc",
      file: "native/lfw/core/js_num.cpp",
      from: "double js_ceil(double x) { return std::ceil(x); }",
      to: "double js_ceil(double x) { return std::trunc(x); }",
    },
    {
      note: "js_ceil: 换成 floor",
      file: "native/lfw/core/js_num.cpp",
      from: "double js_ceil(double x) { return std::ceil(x); }",
      to: "double js_ceil(double x) { return std::floor(x); }",
    },
    {
      note: "js_abs: 丢掉取绝对值",
      file: "native/lfw/core/js_num.cpp",
      from: "double js_abs(double x) { return std::fabs(x); }",
      to: "double js_abs(double x) { return x; }",
    },

    // ---- js_num.cpp: ToUint32 / ToInt32 ----
    {
      note: "js_to_uint32: 截断换成四舍五入",
      file: "native/lfw/core/js_num.cpp",
      from: "  const double t = std::trunc(x);",
      to: "  const double t = std::round(x);",
    },
    {
      note: "js_to_uint32: 截断换成向下取整",
      file: "native/lfw/core/js_num.cpp",
      from: "  const double t = std::trunc(x);",
      to: "  const double t = std::floor(x);",
    },
    {
      note: "js_to_uint32: 模数写错（2^32 → 2^32-1）",
      file: "native/lfw/core/js_num.cpp",
      from: "  double m = std::fmod(t, 4294967296.0);",
      to: "  double m = std::fmod(t, 4294967295.0);",
    },
    {
      note: "js_to_uint32: 负值回绕加错常数",
      file: "native/lfw/core/js_num.cpp",
      from: "  if (m < 0.0) m += 4294967296.0;",
      to: "  if (m < 0.0) m += 4294967295.0;",
    },
    {
      note: "js_to_int32: 负支减错模数",
      file: "native/lfw/core/js_num.cpp",
      from: "             : static_cast<int32_t>(static_cast<int64_t>(u) - 4294967296ll);",
      to: "             : static_cast<int32_t>(static_cast<int64_t>(u) - 4294967295ll);",
    },

    // ---- js_string.cpp: is_str_white_space 常量表逐项 ----
    {
      note: "is_str_white_space: 0x0009 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x0009:",
      to: "    case 0x0001:",
    },
    {
      note: "is_str_white_space: 0x000a 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x000a:",
      to: "    case 0x0002:",
    },
    {
      note: "is_str_white_space: 0x000b 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x000b:",
      to: "    case 0x0003:",
    },
    {
      note: "is_str_white_space: 0x000c 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x000c:",
      to: "    case 0x0004:",
    },
    {
      note: "is_str_white_space: 0x000d 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x000d:",
      to: "    case 0x0005:",
    },
    {
      note: "is_str_white_space: 0x0020 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x0020:",
      to: "    case 0x0006:",
    },
    {
      note: "is_str_white_space: 0x00a0 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x00a0:",
      to: "    case 0x0007:",
    },
    {
      note: "is_str_white_space: 0x1680 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x1680:",
      to: "    case 0x0008:",
    },
    {
      note: "is_str_white_space: 0x2028 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x2028:",
      to: "    case 0x000e:",
    },
    {
      note: "is_str_white_space: 0x2029 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x2029:",
      to: "    case 0x000f:",
    },
    {
      note: "is_str_white_space: 0x202f 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x202f:",
      to: "    case 0x0010:",
    },
    {
      note: "is_str_white_space: 0x205f 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x205f:",
      to: "    case 0x0011:",
    },
    {
      note: "is_str_white_space: 0x3000 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0x3000:",
      to: "    case 0x0012:",
    },
    {
      note: "is_str_white_space: 0xfeff 写错",
      file: "native/lfw/core/js_string.cpp",
      from: "    case 0xfeff:",
      to: "    case 0x0013:",
    },
    {
      note: "is_str_white_space: 0x2000-0x200a 上界改开区间",
      file: "native/lfw/core/js_string.cpp",
      from: "      return c >= 0x2000 && c <= 0x200a;",
      to: "      return c >= 0x2000 && c < 0x200a;",
    },
    {
      note: "is_str_white_space: 0x2000-0x200a 下界改开区间",
      file: "native/lfw/core/js_string.cpp",
      from: "      return c >= 0x2000 && c <= 0x200a;",
      to: "      return c > 0x2000 && c <= 0x200a;",
    },
    {
      note: "is_str_white_space: 0x2000-0x200a 上界放宽到 200b",
      file: "native/lfw/core/js_string.cpp",
      from: "      return c >= 0x2000 && c <= 0x200a;",
      to: "      return c >= 0x2000 && c <= 0x200b;",
    },

    // ---- js_string.cpp: digit_value ----
    {
      note: "digit_value: '9' 不算数字",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (c >= u'0' && c <= u'9') return static_cast<int>(c - u'0');",
      to: "  if (c >= u'0' && c <= u'8') return static_cast<int>(c - u'0');",
    },
    {
      note: "digit_value: 'f' 不算数字",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (c >= u'a' && c <= u'f') return static_cast<int>(c - u'a') + 10;",
      to: "  if (c >= u'a' && c <= u'e') return static_cast<int>(c - u'a') + 10;",
    },
    {
      note: "digit_value: 小写十六进制偏移 +11",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (c >= u'a' && c <= u'f') return static_cast<int>(c - u'a') + 10;",
      to: "  if (c >= u'a' && c <= u'f') return static_cast<int>(c - u'a') + 11;",
    },
    {
      note: "digit_value: 'F' 不算数字",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (c >= u'A' && c <= u'F') return static_cast<int>(c - u'A') + 10;",
      to: "  if (c >= u'A' && c <= u'E') return static_cast<int>(c - u'A') + 10;",
    },
    {
      note: "digit_value: 大写十六进制偏移 +11",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (c >= u'A' && c <= u'F') return static_cast<int>(c - u'A') + 10;",
      to: "  if (c >= u'A' && c <= u'F') return static_cast<int>(c - u'A') + 11;",
    },
    {
      note: "digit_value: 非法字符返回 0",
      file: "native/lfw/core/js_string.cpp",
      from: "  return -1;",
      to: "  return 0;",
    },

    // ---- js_string.cpp: parse_radix ----
    {
      note: "parse_radix: 空前缀守卫放宽",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (from >= n) return nan_v();",
      to: "  if (from > n) return nan_v();",
    },
    {
      note: "parse_radix: 基数上界改开区间",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (d < 0 || static_cast<uint64_t>(d) >= base) return nan_v();",
      to: "    if (d < 0 || static_cast<uint64_t>(d) > base) return nan_v();",
    },
    {
      note: "parse_radix: sticky 掩码少一位",
      file: "native/lfw/core/js_string.cpp",
      from: "  const uint64_t mask = (uint64_t(1) << log2base) - 1;",
      to: "  const uint64_t mask = (uint64_t(1) << log2base);",
    },
    {
      note: "parse_radix: sticky 判定取反",
      file: "native/lfw/core/js_string.cpp",
      from: "      if ((acc & mask) != 0) sticky = true;",
      to: "      if ((acc & mask) == 0) sticky = true;",
    },
    {
      note: "parse_radix: 移出位数每次只记 1",
      file: "native/lfw/core/js_string.cpp",
      from: "      shifted += static_cast<size_t>(log2base);",
      to: "      shifted += 1;",
    },
    {
      note: "parse_radix: 全零返回 NaN",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (acc == 0) return 0.0;",
      to: "  if (acc == 0) return nan_v();",
    },
    {
      note: "parse_radix: 前导位索引差 1",
      file: "native/lfw/core/js_string.cpp",
      from: "  const int lead = 63 - std::countl_zero(acc);",
      to: "  const int lead = 64 - std::countl_zero(acc);",
    },
    {
      note: "parse_radix: 丢弃位数差 1",
      file: "native/lfw/core/js_string.cpp",
      from: "  const int drop = lead - 52;",
      to: "  const int drop = lead - 51;",
    },
    {
      note: "parse_radix: 半值常数差 1 位",
      file: "native/lfw/core/js_string.cpp",
      from: "  const uint64_t half = uint64_t(1) << (drop - 1);",
      to: "  const uint64_t half = uint64_t(1) << drop;",
    },
    {
      note: "parse_radix: 进位阈值改闭区间",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (rest > half || (rest == half && ((keep & 1) != 0 || sticky))) ++rounded;",
      to: "  if (rest >= half || (rest == half && ((keep & 1) != 0 || sticky))) ++rounded;",
    },
    {
      note: "parse_radix: tie 的奇偶判据取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (rest > half || (rest == half && ((keep & 1) != 0 || sticky))) ++rounded;",
      to: "  if (rest > half || (rest == half && ((keep & 1) == 0 || sticky))) ++rounded;",
    },
    {
      note: "parse_radix: sticky 改成与 keep 奇偶相与",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (rest > half || (rest == half && ((keep & 1) != 0 || sticky))) ++rounded;",
      to: "  if (rest > half || (rest == half && ((keep & 1) != 0 && sticky))) ++rounded;",
    },
    {
      note: "parse_radix: 进位初值就 +1",
      file: "native/lfw/core/js_string.cpp",
      from: "  uint64_t rounded = keep;",
      to: "  uint64_t rounded = keep + 1;",
    },
    {
      note: "parse_radix: 结果指数 +1",
      file: "native/lfw/core/js_string.cpp",
      from: "  return std::ldexp(static_cast<double>(rounded), drop + static_cast<int>(shifted));",
      to: "  return std::ldexp(static_cast<double>(rounded), drop + static_cast<int>(shifted) + 1);",
    },
    {
      note: "parse_radix: 结果用未进位值",
      file: "native/lfw/core/js_string.cpp",
      from: "  return std::ldexp(static_cast<double>(rounded), drop + static_cast<int>(shifted));",
      to: "  return std::ldexp(static_cast<double>(keep), drop + static_cast<int>(shifted));",
    },

    // ---- js_string.cpp: scan_decimal ----
    {
      note: "scan_decimal: 符号位记成 '+'",
      file: "native/lfw/core/js_string.cpp",
      from: "    r.neg = (s[i] == u'-');",
      to: "    r.neg = (s[i] == u'+');",
    },
    {
      note: "scan_decimal: Infinity 长度判定放宽",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (n - i == 8) {",
      to: "  if (n - i >= 8) {",
    },
    {
      note: "scan_decimal: Infinity 匹配结果取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (same) {",
      to: "    if (!same) {",
    },
    {
      note: "scan_decimal: 小数点认成逗号",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i < n && s[i] == u'.') {",
      to: "  if (i < n && s[i] == u',') {",
    },
    {
      note: "scan_decimal: 指数 e/E 判定改成 &&",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i < n && (s[i] == u'e' || s[i] == u'E')) {",
      to: "  if (i < n && (s[i] == u'e' && s[i] == u'E')) {",
    },
    {
      note: "scan_decimal: 指数无数字守卫取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (!edig) return r;",
      to: "    if (edig) return r;",
    },
    {
      note: "scan_decimal: 指数符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    r.exp = eneg ? -e : e;",
      to: "    r.exp = eneg ? e : -e;",
    },
    {
      note: "scan_decimal: 结尾多余字符守卫取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i != n) return r;",
      to: "  if (i == n) return r;",
    },
    {
      note: "scan_decimal: 整数部分首位有效数字判定取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;\n    ++sig_index;\n    ++r.int_digits;",
      to: "    if (r.first_sig == static_cast<size_t>(-1) && s[i] == u'0') r.first_sig = sig_index;\n    ++sig_index;\n    ++r.int_digits;",
    },
    {
      note: "scan_decimal: 小数部分首位有效数字判定取反",
      file: "native/lfw/core/js_string.cpp",
      from: "      if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;",
      to: "      if (r.first_sig == static_cast<size_t>(-1) && s[i] == u'0') r.first_sig = sig_index;",
    },
    {
      note: "scan_decimal: 整数循环不认 '9'",
      file: "native/lfw/core/js_string.cpp",
      from: "  while (i < n && s[i] >= u'0' && s[i] <= u'9') {\n    if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;\n    ++sig_index;\n    ++r.int_digits;",
      to: "  while (i < n && s[i] >= u'0' && s[i] < u'9') {\n    if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;\n    ++sig_index;\n    ++r.int_digits;",
    },
    {
      note: "scan_decimal: 小数循环不认 '9'",
      file: "native/lfw/core/js_string.cpp",
      from: "    while (i < n && s[i] >= u'0' && s[i] <= u'9') {\n      if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;\n      ++sig_index;\n      ++i;",
      to: "    while (i < n && s[i] >= u'0' && s[i] < u'9') {\n      if (r.first_sig == static_cast<size_t>(-1) && s[i] != u'0') r.first_sig = sig_index;\n      ++sig_index;\n      ++i;",
    },
    {
      note: "scan_decimal: 正号分支消失",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i < n && (s[i] == u'+' || s[i] == u'-')) {\n    r.neg = (s[i] == u'-');",
      to: "  if (i < n && (s[i] == u'-' || s[i] == u'-')) {\n    r.neg = (s[i] == u'-');",
    },
    {
      note: "scan_decimal: 指数正号分支消失",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (i < n && (s[i] == u'+' || s[i] == u'-')) {",
      to: "    if (i < n && (s[i] == u'-' || s[i] == u'-')) {",
    },

    // ---- js_string.cpp: string_to_number ----
    {
      note: "string_to_number: 全空白返回 NaN",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (b == e) return 0.0;",
      to: "  if (b == e) return nan_v();",
    },
    {
      note: "string_to_number: 前导空白判断用了尾部字符",
      file: "native/lfw/core/js_string.cpp",
      from: "  while (b < e && is_str_white_space(raw[b])) ++b;",
      to: "  while (b < e && is_str_white_space(raw[e - 1])) ++b;",
    },
    {
      note: "string_to_number: 尾部空白读到界外",
      file: "native/lfw/core/js_string.cpp",
      from: "  while (e > b && is_str_white_space(raw[e - 1])) --e;",
      to: "  while (e > b && is_str_white_space(raw[e])) --e;",
    },
    {
      note: "string_to_number: 0 前缀判定取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (n >= 2 && s[0] == u'0') {",
      to: "  if (n >= 2 && s[0] != u'0') {",
    },
    {
      note: "string_to_number: 大写 X 不认",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 4);",
      to: "    if (p == u'x' || p == u'x') return parse_radix(s, 2, 16, 4);",
    },
    {
      note: "string_to_number: 十六进制起点差 1",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 4);",
      to: "    if (p == u'x' || p == u'X') return parse_radix(s, 3, 16, 4);",
    },
    {
      note: "string_to_number: 十六进制基数写成 10",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 4);",
      to: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 10, 4);",
    },
    {
      note: "string_to_number: 十六进制 log2base 写成 8",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 4);",
      to: "    if (p == u'x' || p == u'X') return parse_radix(s, 2, 16, 8);",
    },
    {
      note: "string_to_number: 八进制 log2base 写成 2",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'o' || p == u'O') return parse_radix(s, 2, 8, 3);",
      to: "    if (p == u'o' || p == u'O') return parse_radix(s, 2, 8, 2);",
    },
    {
      note: "string_to_number: 二进制基数写成 4",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (p == u'b' || p == u'B') return parse_radix(s, 2, 2, 1);",
      to: "    if (p == u'b' || p == u'B') return parse_radix(s, 2, 4, 2);",
    },
    {
      note: "string_to_number: 扫描失败返回 0",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (!sc.ok) return nan_v();",
      to: "  if (!sc.ok) return 0.0;",
    },
    {
      note: "string_to_number: Infinity 符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (sc.infinity) return sc.neg ? -std::numeric_limits<double>::infinity()",
      to: "  if (sc.infinity) return sc.neg ? std::numeric_limits<double>::infinity()",
    },
    {
      note: "string_to_number: 正号跳过条件写反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (*begin == '+') ++begin;",
      to: "  if (*begin == '-') ++begin;",
    },
    {
      note: "string_to_number: from_chars 终点少 1",
      file: "native/lfw/core/js_string.cpp",
      from: "  const char* end = ascii.c_str() + n;",
      to: "  const char* end = ascii.c_str() + n - 1;",
    },
    {
      note: "string_to_number: 溢出时上下界取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    const double mag = e10 >= 0 ? std::numeric_limits<double>::infinity() : 0.0;",
      to: "    const double mag = e10 >= 0 ? 0.0 : std::numeric_limits<double>::infinity();",
    },
    {
      note: "string_to_number: 溢出结果符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    return sc.neg ? -mag : mag;",
      to: "    return sc.neg ? mag : -mag;",
    },
    {
      note: "string_to_number: 尾部字符校验取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (r.ec != std::errc() || r.ptr != end) return nan_v();",
      to: "  if (r.ec != std::errc() || r.ptr == end) return nan_v();",
    },
    {
      note: "string_to_number: e10 的 first_sig 项符号写反",
      file: "native/lfw/core/js_string.cpp",
      from: "    const int64_t e10 = static_cast<int64_t>(sc.int_digits) - 1 -\n                        static_cast<int64_t>(sc.first_sig) + sc.exp;",
      to: "    const int64_t e10 = static_cast<int64_t>(sc.int_digits) - 1 +\n                        static_cast<int64_t>(sc.first_sig) + sc.exp;",
    },
    {
      note: "string_to_number: 溢出分支判定取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (r.ec == std::errc::result_out_of_range) {",
      to: "  if (r.ec != std::errc::result_out_of_range) {",
    },

    // ---- js_string.cpp: number_to_string ----
    {
      note: "number_to_string: NaN 拼成小写",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (std::isnan(v)) return u\"NaN\";\n  if (v == 0.0) return u\"0\";",
      to: "  if (std::isnan(v)) return u\"nan\";\n  if (v == 0.0) return u\"0\";",
    },
    {
      note: "number_to_string: 零返回空串",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (v == 0.0) return u\"0\";",
      to: "  if (v == 0.0) return u\"\";",
    },
    {
      note: "number_to_string: Infinity 符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (v == 0.0) return u\"0\";\n  if (std::isinf(v)) return v < 0 ? u\"-Infinity\" : u\"Infinity\";",
      to: "  if (v == 0.0) return u\"0\";\n  if (std::isinf(v)) return v < 0 ? u\"Infinity\" : u\"-Infinity\";",
    },
    {
      note: "number_to_string: 小数点位置少 1",
      file: "native/lfw/core/js_string.cpp",
      from: "  const int n = e10 + 1;",
      to: "  const int n = e10;",
    },
    {
      note: "number_to_string: 5.b 判据 k <= n 改 k < n",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (k <= n && n <= 21) {",
      to: "  if (k < n && n <= 21) {",
    },
    {
      note: "number_to_string: 5.b 上界改成 20",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (k <= n && n <= 21) {",
      to: "  if (k <= n && n <= 20) {",
    },
    {
      note: "number_to_string: 5.c 下界改 n >= 0",
      file: "native/lfw/core/js_string.cpp",
      from: "  } else if (n > 0 && n <= 21) {",
      to: "  } else if (n >= 0 && n <= 21) {",
    },
    {
      note: "number_to_string: 5.d 下界改 n >= -6",
      file: "native/lfw/core/js_string.cpp",
      from: "  } else if (n > -6 && n <= 0) {",
      to: "  } else if (n >= -6 && n <= 0) {",
    },
    {
      note: "number_to_string: 5.d 上界改 n < 0",
      file: "native/lfw/core/js_string.cpp",
      from: "  } else if (n > -6 && n <= 0) {",
      to: "  } else if (n > -6 && n < 0) {",
    },
    {
      note: "number_to_string: 5.d 只补 0 不补小数点",
      file: "native/lfw/core/js_string.cpp",
      from: "    out.push_back(u'0');\n    out.push_back(u'.');\n    out.append(static_cast<size_t>(-n), u'0');",
      to: "    out.push_back(u'0');\n    out.append(static_cast<size_t>(-n), u'0');",
    },
    {
      note: "number_to_string: 5.b 补零个数写成 n",
      file: "native/lfw/core/js_string.cpp",
      from: "    out.append(static_cast<size_t>(n - k), u'0');",
      to: "    out.append(static_cast<size_t>(n), u'0');",
    },
    {
      note: "number_to_string: 5.c 整数位少写 1",
      file: "native/lfw/core/js_string.cpp",
      from: "    push_digits(d.data(), static_cast<size_t>(n));",
      to: "    push_digits(d.data(), static_cast<size_t>(n) - 1);",
    },
    {
      note: "number_to_string: 5.e 指数符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "    out.push_back(ex < 0 ? u'-' : u'+');",
      to: "    out.push_back(ex < 0 ? u'+' : u'-');",
    },
    {
      note: "number_to_string: 5.e 指数少 1",
      file: "native/lfw/core/js_string.cpp",
      from: "    const int ex = n - 1;",
      to: "    const int ex = n;",
    },
    {
      note: "number_to_string: 5.e 单数字也补小数点",
      file: "native/lfw/core/js_string.cpp",
      from: "    if (k > 1) {",
      to: "    if (k >= 1) {",
    },
    {
      note: "number_to_string: 5.e 指数绝对值丢了取负",
      file: "native/lfw/core/js_string.cpp",
      from: "    int ax = ex < 0 ? -ex : ex;",
      to: "    int ax = ex;",
    },
    {
      note: "number_to_string: 5.e 尾数丢首位",
      file: "native/lfw/core/js_string.cpp",
      from: "      push_digits(d.data() + 1, d.size() - 1);",
      to: "      push_digits(d.data(), d.size() - 1);",
    },
    {
      note: "number_to_string: 负号条件取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (neg) out.push_back(u'-');\n\n  const auto push_digits",
      to: "  if (!neg) out.push_back(u'-');\n\n  const auto push_digits",
    },

    // ---- js_string.cpp: shortest_digits ----
    {
      note: "shortest_digits: 小数点认成逗号",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i < n && buf[i] == '.') {",
      to: "  if (i < n && buf[i] == ',') {",
    },
    {
      note: "shortest_digits: 尾数扫描不看 'e'",
      file: "native/lfw/core/js_string.cpp",
      from: "    while (i < n && buf[i] != 'e') digits.push_back(buf[i++]);",
      to: "    while (i < n && buf[i] != 'E') digits.push_back(buf[i++]);",
    },
    {
      note: "shortest_digits: 指数标记认成 'E'",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (i < n && buf[i] == 'e') {",
      to: "  if (i < n && buf[i] == 'E') {",
    },
    {
      note: "shortest_digits: 指数符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  exp10 = eneg ? -e : e;",
      to: "  exp10 = eneg ? e : -e;",
    },

    // ---- number_to_fixed_1（JS `Number.prototype.toFixed(1)`）------------------
    {
      note: "toFixed(1): NaN 不走 Number::toString",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (std::isnan(v)) return u\"NaN\";\n  if (std::isinf(v)) return v < 0 ? u\"-Infinity\" : u\"Infinity\";",
      to: "  if (std::isinf(v)) return v < 0 ? u\"-Infinity\" : u\"Infinity\";",
    },
    {
      note: "toFixed(1): ±Infinity 的符号取反",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (std::isnan(v)) return u\"NaN\";\n  if (std::isinf(v)) return v < 0 ? u\"-Infinity\" : u\"Infinity\";",
      to: "  if (std::isnan(v)) return u\"NaN\";\n  if (std::isinf(v)) return v < 0 ? u\"Infinity\" : u\"-Infinity\";",
    },
    {
      note: "toFixed(1): 1e21 分界的 >= 写成 >",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (a >= 1e21) return number_to_string(v);",
      to: "  if (a > 1e21) return number_to_string(v);",
    },
    {
      note: "toFixed(1): 1e21 分界的阈值写成 1e20",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (a >= 1e21) return number_to_string(v);",
      to: "  if (a >= 1e20) return number_to_string(v);",
    },
    {
      note: "toFixed(1): ≥1e21 时丢掉符号",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (a >= 1e21) return number_to_string(v);",
      to: "  if (a >= 1e21) return number_to_string(a);",
    },
    {
      note: "toFixed(1): 零支写成 \"0\"（少一位小数）",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (a == 0.0) return u\"0.0\";",
      to: "  if (a == 0.0) return u\"0\";",
    },
    {
      note: "toFixed(1): 整数支不补 \".0\"",
      file: "native/lfw/core/js_string.cpp",
      from: "    for (const char* p = buf; p != r.ptr; ++p) out.push_back(static_cast<char16_t>(*p));\n    out.push_back(u'.');\n    out.push_back(u'0');\n    return out;",
      to: "    for (const char* p = buf; p != r.ptr; ++p) out.push_back(static_cast<char16_t>(*p));\n    return out;",
    },
    {
      note: "toFixed(1): 整数支用最短往返格式（大数会变指数串）",
      file: "native/lfw/core/js_string.cpp",
      from: "    const std::to_chars_result r =\n        std::to_chars(buf, buf + sizeof buf, a, std::chars_format::fixed, 0);",
      to: "    const std::to_chars_result r = std::to_chars(buf, buf + sizeof buf, a);",
    },
    {
      note: "toFixed(1): 隐式 1 位没算进尾数",
      file: "native/lfw/core/js_string.cpp",
      from: "  const uint64_t m = ex == 0 ? frac : frac | (1ull << 52);",
      to: "  const uint64_t m = frac;",
    },
    {
      note: "toFixed(1): 指数偏移少 1（ex - 1075 → ex - 1074）",
      file: "native/lfw/core/js_string.cpp",
      from: "  const int e = ex == 0 ? -1074 : ex - 1075;",
      to: "  const int e = ex == 0 ? -1074 : ex - 1074;",
    },
    {
      note: "toFixed(1): 进位公式少了 2N（用 N 顶替）",
      file: "native/lfw/core/js_string.cpp",
      from: "    const uint64_t num = m * 20ull + (1ull << k);",
      to: "    const uint64_t num = m * 10ull + (1ull << k);",
    },
    {
      note: "toFixed(1): 进位公式的右移少 1 位",
      file: "native/lfw/core/js_string.cpp",
      from: "    n = num >> (k + 1);",
      to: "    n = num >> k;",
    },
    {
      note: "toFixed(1): 非零下界收到 k <= 56（.06 这类值会掉成 0.0）",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (k <= 58) {",
      to: "  if (k <= 56) {",
    },
    {
      note: "toFixed(1): 一位数字少了前导 0",
      file: "native/lfw/core/js_string.cpp",
      from: "  if (digits.size() == 1) {\n    out.push_back(u'0');",
      to: "  if (digits.size() == 0) {\n    out.push_back(u'0');",
    },
    {
      note: "toFixed(1): 小数点插到最前面",
      file: "native/lfw/core/js_string.cpp",
      from: "  digits.insert(digits.size() - 1, 1, '.');",
      to: "  digits.insert(0, 1, '.');",
    },
  ],
};
