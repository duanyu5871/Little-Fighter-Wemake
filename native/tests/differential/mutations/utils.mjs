/**
 * utils 变异规格
 *
 * 覆盖 subject: utils（cross_bounding / easing / Times / utf8 / type_check / type_cast）
 *
 * 已删除的等价变异（不算覆盖缺口，理由记在此处以免以后重复尝试）：
 *  1. type_check.h  `is_num(double)`: `!std::isnan(d) && std::isfinite(d)` 去掉 `!std::isnan(d)` ——
 *     对有限 double，`std::isfinite(d)` 已经排除 NaN ⇒ 两者恒等。
 *  2. type_check.h  `is_int(double)`: `std::trunc(d) == d` 换成 `std::floor(d) == d` ——
 *     对有限 d，两者都等价于"d 是整数"。
 *  3. type_check.h  `is_non_empty_str`: `truthy(v) && is_str(v)` 交换两个合取项顺序 —— 恒等。
 *  4. ease_in_out_quint.h  `factor < 0.5` 换成 `factor <= 0.5` —— 两分支在 factor == 0.5 处
 *     恰好同值（`16 * 0.5^5 == 1 - 1^5 / 2 == 0.5`），无法区分。
 *  5. cross_bounding.h  任一 `max(r0.x, r1.x)` / `min(r0.x, r1.x)` 交换 r0/r1 —— max/min 可交换。
 *  6. ease_in_out_quint.h  backward 的 `ratio < 0.5` 换成 `ratio <= 0.5` ——
 *     ratio == 0.5 时下支 `pow(0.5 / 16, 0.2) == pow(1/32, 0.2) == 0.5`，上支 `1 - pow(1, 0.2)/2 == 0.5`，
 *     两值相等（与第 4 条同源：0.5 是这条曲线的不动点）。
 *  7. times.h  `_value` 的类内成员初值（`= MIN` 改成别的）—— 构造器体里 `set_range` 会立刻写 `_value`，
 *     构造完必定被覆盖 ⇒ 死存储（TS 侧同理，`constructor` 也调 `set_range`）。
 *     注意 `_lifes`/`_remains` 的初值 **不** 被 `set_range` 碰，所以它们是可观测的。
 *  8. times.cpp  `add()` 的 `if (ret && _remains > 0.0)` 把 `> 0.0` 改成 `>= 0.0` ——
 *     能走到这一行就说明 `_remains != 0`（前面已有 `if (_remains == 0.0) return false;`）⇒ 两条件等价。
 */
export default {
  subject: "utils",
  mutations: [
    // ---- cross_bounding.h ----
    {
      note: "cross_bounding: left 用 min",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.left = max(r0.left, r1.left);",
      to: "  ret.left = min(r0.left, r1.left);",
    },
    {
      note: "cross_bounding: right 用 max",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.right = min(r0.right, r1.right);",
      to: "  ret.right = max(r0.right, r1.right);",
    },
    {
      note: "cross_bounding: bottom 用 min",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.bottom = max(r0.bottom, r1.bottom);",
      to: "  ret.bottom = min(r0.bottom, r1.bottom);",
    },
    {
      note: "cross_bounding: top 用 max",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.top = min(r0.top, r1.top);",
      to: "  ret.top = max(r0.top, r1.top);",
    },
    {
      note: "cross_bounding: far 用 min",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.far = max(r0.far, r1.far);",
      to: "  ret.far = min(r0.far, r1.far);",
    },
    {
      note: "cross_bounding: near 用 max",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.near = min(r0.near, r1.near);",
      to: "  ret.near = max(r0.near, r1.near);",
    },
    {
      note: "cross_bounding: far 取错了源字段（near）",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.far = max(r0.far, r1.far);",
      to: "  ret.far = max(r0.near, r1.near);",
    },
    {
      note: "cross_bounding: near 取错了源字段（far）",
      file: "native/lfw/utils/cross_bounding.h",
      from: "  ret.near = min(r0.near, r1.near);",
      to: "  ret.near = min(r0.far, r1.far);",
    },

    // ---- easing: 默认参数 ----
    {
      note: "ease_linearity: 默认 from 写成 1",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "inline double ease_linearity(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_linearity(double factor, double from = 1.0, double to = 1.0) {",
    },
    {
      note: "ease_linearity: 默认 to 写成 0",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "inline double ease_linearity(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_linearity(double factor, double from = 0.0, double to = 0.0) {",
    },
    {
      note: "ease_linearity_backward: 默认 to 写成 0",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "inline double ease_linearity_backward(double v, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_linearity_backward(double v, double from = 0.0, double to = 0.0) {",
    },
    {
      note: "ease_in_out_sine: 默认 from 写成 1",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from:
        "inline double ease_in_out_sine(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_in_out_sine(double factor, double from = 1.0, double to = 1.0) {",
    },
    {
      note: "ease_in_out_sine: 默认 to 写成 0",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from:
        "inline double ease_in_out_sine(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_in_out_sine(double factor, double from = 0.0, double to = 0.0) {",
    },
    {
      note: "ease_in_out_quint: 默认 from 写成 1",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from:
        "inline double ease_in_out_quint(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_in_out_quint(double factor, double from = 1.0, double to = 1.0) {",
    },
    {
      note: "ease_in_out_quint: 默认 to 写成 0",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from:
        "inline double ease_in_out_quint(double factor, double from = 0.0, double to = 1.0) {",
      to: "inline double ease_in_out_quint(double factor, double from = 0.0, double to = 0.0) {",
    },

    // ---- ease_linearity ----
    {
      note: "ease_linearity: (to - from) 反了",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "  return from + (to - from) * factor;",
      to: "  return from + (from - to) * factor;",
    },
    {
      note: "ease_linearity: 加法写成减法",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "  return from + (to - from) * factor;",
      to: "  return from - (to - from) * factor;",
    },
    {
      note: "ease_linearity_backward: 除法写成乘法",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "  return (v - from) / (to - from);",
      to: "  return (v - from) * (to - from);",
    },
    {
      note: "ease_linearity_backward: v - from 写成 v - to",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "  return (v - from) / (to - from);",
      to: "  return (v - to) / (to - from);",
    },
    {
      note: "ease_linearity_backward: 分母取反",
      file: "native/lfw/utils/easing/ease_linearity.h",
      from: "  return (v - from) / (to - from);",
      to: "  return (v - from) / (from - to);",
    },

    // ---- ease_in_out_sine ----
    {
      note: "ease_in_out_sine: from - ... 写成 from + ...",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  return from - ((to - from) * (cos(PI * factor) - 1.0)) / 2.0;",
      to: "  return from + ((to - from) * (cos(PI * factor) - 1.0)) / 2.0;",
    },
    {
      note: "ease_in_out_sine: cos(...) - 1 写成 + 1",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  return from - ((to - from) * (cos(PI * factor) - 1.0)) / 2.0;",
      to: "  return from - ((to - from) * (cos(PI * factor) + 1.0)) / 2.0;",
    },
    {
      note: "ease_in_out_sine: PI * factor 写成 PI / factor",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  return from - ((to - from) * (cos(PI * factor) - 1.0)) / 2.0;",
      to: "  return from - ((to - from) * (cos(PI / factor) - 1.0)) / 2.0;",
    },
    {
      note: "ease_in_out_sine_backward: acos 里 + 1 写成 - 1",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  return acos((2.0 * (from - v)) / (to - from) + 1.0) / PI;",
      to: "  return acos((2.0 * (from - v)) / (to - from) - 1.0) / PI;",
    },
    {
      note: "ease_in_out_sine_backward: 下界不夹",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  if (v < lo) v = lo;\n  if (v > hi) v = hi;",
      to: "  if (v > hi) v = hi;",
    },
    {
      note: "ease_in_out_sine_backward: 上界不夹",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  if (v < lo) v = lo;\n  if (v > hi) v = hi;",
      to: "  if (v < lo) v = lo;",
    },
    {
      note: "ease_in_out_sine_backward: lo 用 max",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from:
        "  const double lo = min(from, to);\n  const double hi = max(from, to);\n  if (v < lo) v = lo;",
      to: "  const double lo = max(from, to);\n  const double hi = max(from, to);\n  if (v < lo) v = lo;",
    },
    {
      note: "ease_in_out_sine_backward: 夹到 hi 而不是 lo",
      file: "native/lfw/utils/easing/ease_in_out_sine.h",
      from: "  if (v < lo) v = lo;",
      to: "  if (v < lo) v = hi;",
    },

    // ---- ease_in_out_quint ----
    {
      note: "ease_in_out_quint: 5 次方写成 4 次方",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 - pow(-2.0 * factor + 2.0, 5.0) / 2.0;",
      to: "      factor < 0.5 ? 16.0 * pow(factor, 4.0) : 1.0 - pow(-2.0 * factor + 2.0, 5.0) / 2.0;",
    },
    {
      note: "ease_in_out_quint: -2f+2 写成 2f+2",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 - pow(-2.0 * factor + 2.0, 5.0) / 2.0;",
      to: "      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 - pow(2.0 * factor + 2.0, 5.0) / 2.0;",
    },
    {
      note: "ease_in_out_quint: 1 - ... 写成 1 + ...",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 - pow(-2.0 * factor + 2.0, 5.0) / 2.0;",
      to: "      factor < 0.5 ? 16.0 * pow(factor, 5.0) : 1.0 + pow(-2.0 * factor + 2.0, 5.0) / 2.0;",
    },
    {
      note: "ease_in_out_quint: from + ratio*(to-from) 写成 -",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  return from + ratio * (to - from);",
      to: "  return from - ratio * (to - from);",
    },
    {
      note: "ease_in_out_quint_backward: ratio/16 写成 ratio*16",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "    return pow(ratio / 16.0, 0.2);",
      to: "    return pow(ratio * 16.0, 0.2);",
    },
    {
      note: "ease_in_out_quint_backward: 0.2 次方写成 2",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "    return pow(ratio / 16.0, 0.2);",
      to: "    return pow(ratio / 16.0, 2.0);",
    },
    {
      note: "ease_in_out_quint_backward: (1-ratio) 写成 ratio",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  return 1.0 - pow(2.0 * (1.0 - ratio), 0.2) / 2.0;",
      to: "  return 1.0 - pow(2.0 * ratio, 0.2) / 2.0;",
    },
    {
      note: "ease_in_out_quint_backward: /2 写成 *2",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  return 1.0 - pow(2.0 * (1.0 - ratio), 0.2) / 2.0;",
      to: "  return 1.0 - pow(2.0 * (1.0 - ratio), 0.2) * 2.0;",
    },
    {
      note: "ease_in_out_quint_backward: hi 用 min",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  const double hi = max(from, to);",
      to: "  const double hi = min(from, to);",
    },
    {
      note: "ease_in_out_quint_backward: 夹到 hi 而不是 lo",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  if (v < lo) v = lo;",
      to: "  if (v < lo) v = hi;",
    },
    {
      note: "ease_in_out_quint_backward: 上界不夹",
      file: "native/lfw/utils/easing/ease_in_out_quint.h",
      from: "  if (v < lo) v = lo;\n  if (v > hi) v = hi;",
      to: "  if (v < lo) v = lo;",
    },

    // ---- times.h ----
    {
      note: "Times: MAX 常量末位改 1",
      file: "native/lfw/utils/times.h",
      from: "  static constexpr double MAX = 9007199254740991.0;",
      to: "  static constexpr double MAX = 9007199254740992.0;",
    },
    {
      note: "Times: MIN 常量写成 1",
      file: "native/lfw/utils/times.h",
      from: "  static constexpr double MIN = 0.0;",
      to: "  static constexpr double MIN = 1.0;",
    },
    {
      note: "Times: LIFES 常量写成 0",
      file: "native/lfw/utils/times.h",
      from: "  static constexpr double LIFES = 9007199254740991.0;",
      to: "  static constexpr double LIFES = 0.0;",
    },
    {
      note: "Times::value 返回 _min",
      file: "native/lfw/utils/times.h",
      from: "  double value() const { return _value; }",
      to: "  double value() const { return _min; }",
    },
    {
      note: "Times::min 返回 _value",
      file: "native/lfw/utils/times.h",
      from: "  double min() const { return _min; }",
      to: "  double min() const { return _value; }",
    },
    {
      note: "Times::max 返回 _min",
      file: "native/lfw/utils/times.h",
      from: "  double max() const { return _max; }",
      to: "  double max() const { return _min; }",
    },
    {
      note: "Times::lifes 返回 _remains",
      file: "native/lfw/utils/times.h",
      from: "  double lifes() const { return _lifes; }",
      to: "  double lifes() const { return _remains; }",
    },
    {
      note: "Times::remains 返回 _lifes",
      file: "native/lfw/utils/times.h",
      from: "  double remains() const { return _remains; }",
      to: "  double remains() const { return _lifes; }",
    },
    {
      note: "Times::is_max 用 >",
      file: "native/lfw/utils/times.h",
      from: "  bool is_max() const { return _value >= _max; }",
      to: "  bool is_max() const { return _value > _max; }",
    },
    {
      note: "Times::is_min 用 <",
      file: "native/lfw/utils/times.h",
      from: "  bool is_min() const { return _value <= _min; }",
      to: "  bool is_min() const { return _value < _min; }",
    },
    // ---- times.cpp ----
    {
      note: "Times::set_min 去掉 floor",
      file: "native/lfw/utils/times.cpp",
      from: "void Times::set_min(double v) { _min = lfw::floor(v); }",
      to: "void Times::set_min(double v) { _min = v; }",
    },
    {
      note: "Times::set_max 去掉 floor",
      file: "native/lfw/utils/times.cpp",
      from: "void Times::set_max(double v) { _max = lfw::floor(v); }",
      to: "void Times::set_max(double v) { _max = v; }",
    },
    {
      note: "Times::set_value 去掉 floor",
      file: "native/lfw/utils/times.cpp",
      from: "void Times::set_value(double v) { _value = lfw::floor(v); }",
      to: "void Times::set_value(double v) { _value = v; }",
    },
    {
      note: "Times::reborn 的 _max 写成 MIN",
      file: "native/lfw/utils/times.cpp",
      from: "  _max = MAX;\n  _value = _min = MIN;",
      to: "  _max = MIN;\n  _value = _min = MIN;",
    },
    {
      note: "Times::reborn 的 lifes/remains 写成 MIN",
      file: "native/lfw/utils/times.cpp",
      from: "  _lifes = _remains = LIFES;",
      to: "  _lifes = _remains = MIN;",
    },
    {
      note: "Times::set_range 的 _value 写成 _min",
      file: "native/lfw/utils/times.cpp",
      from: "  _max = lfw::max(a, b);\n  _value = a;",
      to: "  _max = lfw::max(a, b);\n  _value = _min;",
    },
    {
      note: "Times::set_range 的 _min 用 max",
      file: "native/lfw/utils/times.cpp",
      from: "  _min = lfw::min(a, b);\n  _max = lfw::max(a, b);",
      to: "  _min = lfw::max(a, b);\n  _max = lfw::max(a, b);",
    },
    {
      note: "Times::set_range 的 _max 用 min",
      file: "native/lfw/utils/times.cpp",
      from: "  _min = lfw::min(a, b);\n  _max = lfw::max(a, b);",
      to: "  _min = lfw::min(a, b);\n  _max = lfw::min(a, b);",
    },
    {
      note: "Times::set_range 的 b 不 floor",
      file: "native/lfw/utils/times.cpp",
      from: "  const double a = lfw::floor(min_v);\n  const double b = lfw::floor(max_v);",
      to: "  const double a = lfw::floor(min_v);\n  const double b = max_v;",
    },
    {
      note: "Times::set_lifes 去掉 floor",
      file: "native/lfw/utils/times.cpp",
      from: "  _lifes = lfw::floor(v);\n  _remains = _lifes;",
      to: "  _lifes = v;\n  _remains = _lifes;",
    },
    {
      note: "Times::set_lifes 不重置 remains",
      file: "native/lfw/utils/times.cpp",
      from: "  _lifes = lfw::floor(v);\n  _remains = _lifes;",
      to: "  _lifes = lfw::floor(v);",
    },
    {
      note: "Times::reset 的 _value 写成 _max",
      file: "native/lfw/utils/times.cpp",
      from: "  _value = _min;\n  _remains = _lifes;\n  return *this;\n}",
      to: "  _value = _max;\n  _remains = _lifes;\n  return *this;\n}",
    },
    {
      note: "Times::reset 的 _remains 写成 0",
      file: "native/lfw/utils/times.cpp",
      from: "  _value = _min;\n  _remains = _lifes;\n  return *this;\n}",
      to: "  _value = _min;\n  _remains = 0.0;\n  return *this;\n}",
    },
    {
      note: "Times::add 去掉 remains==0 早退",
      file: "native/lfw/utils/times.cpp",
      from: "  if (_remains == 0.0) return false;\n  const double v =",
      to: "  const double v =",
    },
    {
      note: "Times::add 不量化",
      file: "native/lfw/utils/times.cpp",
      from: "  const double v = _value = round_float(_value + d);",
      to: "  const double v = _value = _value + d;",
    },
    {
      note: "Times::add 的 ret 用 >",
      file: "native/lfw/utils/times.cpp",
      from: "  const bool ret = v >= _max;",
      to: "  const bool ret = v > _max;",
    },
    {
      note: "Times::add 的 remains 自增",
      file: "native/lfw/utils/times.cpp",
      from: "  if (ret && _remains > 0.0) --_remains;",
      to: "  if (ret && _remains > 0.0) ++_remains;",
    },
    {
      note: "Times::add 的回绕判断用 >",
      file: "native/lfw/utils/times.cpp",
      from: "  if (v >= _max) _value = _min;",
      to: "  if (v > _max) _value = _min;",
    },
    {
      note: "Times::add 下溢回绕用 <=",
      file: "native/lfw/utils/times.cpp",
      from: "  if (v < _min) _value = _max;",
      to: "  if (v <= _min) _value = _max;",
    },
    {
      note: "Times::add 下溢回绕写成 _min",
      file: "native/lfw/utils/times.cpp",
      from: "  if (v < _min) _value = _max;",
      to: "  if (v < _min) _value = _min;",
    },
    {
      note: "Times::write_nums 的 _max 写成 _min",
      file: "native/lfw/utils/times.cpp",
      from: "  nums[i + 2] = _max;",
      to: "  nums[i + 2] = _min;",
    },
    {
      note: "Times::write_nums 的 _remains 写成 _lifes",
      file: "native/lfw/utils/times.cpp",
      from: "  nums[i + 4] = _remains;",
      to: "  nums[i + 4] = _lifes;",
    },
    {
      note: "Times::read_nums 的 _max 读错下标",
      file: "native/lfw/utils/times.cpp",
      from: "  _value = nums[i];\n  _min = nums[i + 1];\n  _max = nums[i + 2];",
      to: "  _value = nums[i];\n  _min = nums[i + 1];\n  _max = nums[i + 1];",
    },
    {
      note: "Times::read_nums 的 _remains 读错下标",
      file: "native/lfw/utils/times.cpp",
      from: "  _lifes = nums[i + 3];\n  _remains = nums[i + 4];",
      to: "  _lifes = nums[i + 3];\n  _remains = nums[i + 3];",
    },
    {
      note: "Times::to_snapshot 的 lifes/remains 互换",
      file: "native/lfw/utils/times.cpp",
      from: "  return std::array<double, 5>{_value, _min, _max, _lifes, _remains};",
      to: "  return std::array<double, 5>{_value, _min, _max, _remains, _lifes};",
    },
    {
      note: "Times::read_snapshot 的 _lifes 读错下标",
      file: "native/lfw/utils/times.cpp",
      from: "  _lifes = s[3];\n  _remains = s[4];",
      to: "  _lifes = s[2];\n  _remains = s[4];",
    },

    // ---- utf8.cpp: encode ----
    {
      note: "utf8 编码：1 字节长度算成 2",
      file: "native/lfw/utils/utf8.cpp",
      from: "    if (c < 0x80) {\n      byte_len += 1;",
      to: "    if (c < 0x80) {\n      byte_len += 2;",
    },
    {
      note: "utf8 编码：2 字节长度算成 3",
      file: "native/lfw/utils/utf8.cpp",
      from: "    } else if (c < 0x800) {\n      byte_len += 2;",
      to: "    } else if (c < 0x800) {\n      byte_len += 3;",
    },
    {
      note: "utf8 编码：代理对长度多算 1 字节",
      file: "native/lfw/utils/utf8.cpp",
      from: "      byte_len += 4;\n      ++i;",
      to: "      byte_len += 5;\n      ++i;",
    },
    {
      note: "utf8 编码：代理对不上不跳过低位",
      file: "native/lfw/utils/utf8.cpp",
      from: "      byte_len += 4;\n      ++i;",
      to: "      byte_len += 4;",
    },
    {
      note: "utf8 编码：2 字节前导位移写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "      bytes[pos++] = static_cast<uint8_t>(0xc0 | (c >> 6));",
      to: "      bytes[pos++] = static_cast<uint8_t>(0xc0 | (c >> 5));",
    },
    {
      note: "utf8 编码：代理对组合位移写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "      c = 0x10000u + static_cast<uint32_t>(((hi & 0x3ffu) << 10) | (lo & 0x3ffu));",
      to: "      c = 0x10000u + static_cast<uint32_t>(((hi & 0x3ffu) << 11) | (lo & 0x3ffu));",
    },
    {
      note: "utf8 编码：高位代理掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "      c = 0x10000u + static_cast<uint32_t>(((hi & 0x3ffu) << 10) | (lo & 0x3ffu));",
      to: "      c = 0x10000u + static_cast<uint32_t>(((hi & 0x7fffu) << 10) | (lo & 0x3ffu));",
    },
    {
      note: "utf8 编码：3 字节前导位移写错",
      file: "native/lfw/utils/utf8.cpp",
      from:
        "      bytes[pos++] = static_cast<uint8_t>(0xe0 | (c >> 12));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3f));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | (c & 0x3f));\n    }\n  }\n  return bytes;",
      to:
        "      bytes[pos++] = static_cast<uint8_t>(0xe0 | (c >> 6));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3f));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | (c & 0x3f));\n    }\n  }\n  return bytes;",
    },
    {
      note: "utf8 编码：代理对第三字节掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from:
        "      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 12) & 0x3f));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3f));",
      to: "      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 12) & 0x3f));\n      bytes[pos++] = static_cast<uint8_t>(0x80 | ((c >> 5) & 0x3f));",
    },

    // ---- utf8.cpp: decode ----
    {
      note: "utf8 解码：ASCII 判断写成 <=",
      file: "native/lfw/utils/utf8.cpp",
      from: "    if (b0 < 0x80) {\n      out.push_back(static_cast<char16_t>(b0));",
      to: "    if (b0 <= 0x80) {\n      out.push_back(static_cast<char16_t>(b0));",
    },
    {
      note: "utf8 解码：2 字节前导掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "    } else if ((b0 & 0xe0) == 0xc0) {",
      to: "    } else if ((b0 & 0xf0) == 0xc0) {",
    },
    {
      note: "utf8 解码：3 字节前导掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "    } else if ((b0 & 0xf0) == 0xe0) {",
      to: "    } else if ((b0 & 0xe0) == 0xe0) {",
    },
    {
      note: "utf8 解码：4 字节前导掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "    } else if ((b0 & 0xf8) == 0xf0) {",
      to: "    } else if ((b0 & 0xf0) == 0xf0) {",
    },
    {
      note: "utf8 解码：2 字节分支不判越界",
      file: "native/lfw/utils/utf8.cpp",
      from:
        "      const uint8_t b1 = i < len ? buf[i++] : 0;\n      out.push_back(static_cast<char16_t>(((b0 & 0x1fu) << 6) | (b1 & 0x3fu)));",
      to: "      const uint8_t b1 = i < len ? buf[i++] : 0xff;\n      out.push_back(static_cast<char16_t>(((b0 & 0x1fu) << 6) | (b1 & 0x3fu)));",
    },
    {
      note: "utf8 解码：3 字节分支不判越界",
      file: "native/lfw/utils/utf8.cpp",
      from:
        "      const uint8_t b1 = i < len ? buf[i++] : 0;\n      const uint8_t b2 = i < len ? buf[i++] : 0;\n      out.push_back(static_cast<char16_t>(((b0 & 0x0fu) << 12)",
      to: "      const uint8_t b1 = i < len ? buf[i++] : 0xff;\n      const uint8_t b2 = i < len ? buf[i++] : 0xff;\n      out.push_back(static_cast<char16_t>(((b0 & 0x0fu) << 12)",
    },
    {
      note: "utf8 解码：4 字节分支不判越界",
      file: "native/lfw/utils/utf8.cpp",
      from:
        "      const uint8_t b3 = i < len ? buf[i++] : 0;\n      const uint32_t cp = ",
      to: "      const uint8_t b3 = i < len ? buf[i++] : 0xff;\n      const uint32_t cp = ",
    },
    {
      note: "utf8 解码：高位代理位移写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "      out.push_back(static_cast<char16_t>(0xd800u + (off >> 10)));",
      to: "      out.push_back(static_cast<char16_t>(0xd800u + (off >> 11)));",
    },
    {
      note: "utf8 解码：低位代理掩码写错",
      file: "native/lfw/utils/utf8.cpp",
      from: "      out.push_back(static_cast<char16_t>(0xdc00u + (off & 0x3ffu)));",
      to: "      out.push_back(static_cast<char16_t>(0xdc00u + (off & 0x7ffu)));",
    },
    {
      note: "utf8 解码：忘了减 0x10000",
      file: "native/lfw/utils/utf8.cpp",
      from: "      const uint32_t off = cp - 0x10000u;",
      to: "      const uint32_t off = cp;",
    },

    // ---- type_check.h ----
    {
      note: "is_num(double): 接受 Infinity",
      file: "native/lfw/utils/type_check.h",
      from: "inline bool is_num(double d) { return !std::isnan(d) && std::isfinite(d); }",
      to: "inline bool is_num(double d) { return !std::isnan(d); }",
    },
    {
      note: "is_num(Value): 不要求是 double",
      file: "native/lfw/utils/type_check.h",
      from:
        "inline bool is_num(const Value& v) {\n  const double* d = std::get_if<double>(&v);\n  return d != nullptr && is_num(*d);",
      to: "inline bool is_num(const Value& v) {\n  const double* d = std::get_if<double>(&v);\n  return d == nullptr || is_num(*d);",
    },
    {
      note: "is_positive: > 写成 >=",
      file: "native/lfw/utils/type_check.h",
      from: "  return d != nullptr && is_num(*d) && *d > 0;",
      to: "  return d != nullptr && is_num(*d) && *d >= 0;",
    },
    {
      note: "is_positive: 少了 is_num 守卫",
      file: "native/lfw/utils/type_check.h",
      from: "  return d != nullptr && is_num(*d) && *d > 0;",
      to: "  return d != nullptr && *d > 0;",
    },
    {
      note: "not_zero_num: != 0 写成 > 0",
      file: "native/lfw/utils/type_check.h",
      from: "  return d != nullptr && is_num(*d) && *d != 0;",
      to: "  return d != nullptr && is_num(*d) && *d > 0;",
    },
    {
      note: "is_int(double): 去掉有限性检查",
      file: "native/lfw/utils/type_check.h",
      from: "inline bool is_int(double d) { return std::isfinite(d) && std::trunc(d) == d; }",
      to: "inline bool is_int(double d) { return std::trunc(d) == d; }",
    },
    {
      note: "is_int(double): == 写成 >=",
      file: "native/lfw/utils/type_check.h",
      from: "inline bool is_int(double d) { return std::isfinite(d) && std::trunc(d) == d; }",
      to: "inline bool is_int(double d) { return std::isfinite(d) && std::trunc(d) >= d; }",
    },
    {
      note: "is_str: 判成 double",
      file: "native/lfw/utils/type_check.h",
      from: "inline bool is_str(const Value& v) { return std::holds_alternative<std::u16string>(v); }",
      to: "inline bool is_str(const Value& v) { return std::holds_alternative<double>(v); }",
    },
    {
      note: "is_non_empty_str: 丢了非空判断",
      file: "native/lfw/utils/type_check.h",
      from:
        "inline bool is_non_empty_str(const Value& v) { return truthy(v) && is_str(v); }",
      to: "inline bool is_non_empty_str(const Value& v) { return is_str(v); }",
    },

    // ---- type_cast.h ----
    {
      note: "to_num(v): 不判 is_num",
      file: "native/lfw/utils/type_cast.h",
      from: "  return is_num(n) ? std::optional<double>(n) : std::nullopt;",
      to: "  return std::optional<double>(n);",
    },
    {
      note: "to_num(v): 判断写成 !is_num",
      file: "native/lfw/utils/type_cast.h",
      from: "  return is_num(n) ? std::optional<double>(n) : std::nullopt;",
      to: "  return !is_num(n) ? std::optional<double>(n) : std::nullopt;",
    },
    {
      note: "to_num(v, or): 不判 is_num",
      file: "native/lfw/utils/type_cast.h",
      from: "  return is_num(n) ? n : or_value;",
      to: "  return n;",
    },
    {
      note: "to_num(v, or): 判断的是原值不是转换结果",
      file: "native/lfw/utils/type_cast.h",
      from: "  return is_num(n) ? n : or_value;",
      to: "  return is_num(v) ? n : or_value;",
    },
    {
      note: "to_num(v, or): 回落值取反",
      file: "native/lfw/utils/type_cast.h",
      from: "  return is_num(n) ? n : or_value;",
      to: "  return is_num(n) ? or_value : n;",
    },
  ],
};
