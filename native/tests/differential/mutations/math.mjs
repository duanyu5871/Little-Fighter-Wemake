// 已删 / 未写的等价变异（不要重加）：
// 1. `round_float` 里 `n == 0.0` 的提前返回：去掉它对 ±0 完全等价
//    （`round(0 * 1000) / 1000` 仍是 ±0）。只有 multiplier 为 NaN/Infinity 时才不同，
//    而 `math` subject 没有暴露带 multiplier 的 `round_float`，记作不可观察。
// 2. `float_equal` 把 `round_float(abs(x - y))` 的 `abs` 去掉：结果与 `0` 比较，
//    而 `round_float(-0.0004)` 是 **-0**，`-0 == 0` 为真 ⇒ abs 是冗余的。
// 3. `floor_float` 的默认倍数 1000 → 100：它唯一的调用点 `normalize` 总是显式传 p
//    （subject 里的 fallback 就是 1000）⇒ 头文件默认值在本 subject 不可达。
export default {
  subject: "math",
  mutations: [
    {
      note: "clamp：上下界判断互换",
      file: "native/lfw/utils/math/clamp.h",
      from: `  return value < lo ? lo : value > hi ? hi : value;`,
      to: `  return value > lo ? lo : value < hi ? hi : value;`,
    },
    {
      note: "clamp：返回原值而不夹取",
      file: "native/lfw/utils/math/clamp.h",
      from: `  return value < lo ? lo : value > hi ? hi : value;`,
      to: `  return value;`,
    },
    {
      note: "clamp_add：offset 写成减",
      file: "native/lfw/utils/math/clamp_add.h",
      from: `  value = round_float(value + offset);`,
      to: `  value = round_float(value - offset);`,
    },
    {
      note: "clamp_add：不做 round_float",
      file: "native/lfw/utils/math/clamp_add.h",
      from: `  value = round_float(value + offset);`,
      to: `  value = value + offset;`,
    },
    {
      note: "normalize：用 round_float 而不是 floor_float",
      file: "native/lfw/utils/math/normalize.h",
      from: `  n = floor_float(n, p);`,
      to: `  n = round_float(n, p);`,
    },
    {
      note: "normalize：默认倍数写成 100",
      file: "native/lfw/utils/math/normalize.h",
      from: `inline double normalize(double n, double p = 1000.0) {`,
      to: `inline double normalize(double n, double p = 100.0) {`,
    },
    {
      note: "normalize：正零也返回 1",
      file: "native/lfw/utils/math/normalize.h",
      from: `  if (n > 0) return 1.0;`,
      to: `  if (n >= 0) return 1.0;`,
    },
    {
      note: "normalize：正负号取反",
      file: "native/lfw/utils/math/normalize.h",
      from: `  if (n < 0) return -1.0;`,
      to: `  if (n < 0) return 1.0;`,
    },
    {
      note: "equal：用 >= 0",
      file: "native/lfw/utils/math/float_equal.h",
      from: `inline bool equal(double x, double y) { return round_float(x - y) == 0; }`,
      to: `inline bool equal(double x, double y) { return round_float(x - y) >= 0; }`,
    },
    {
      note: "eqgt：用 > 0",
      file: "native/lfw/utils/math/float_equal.h",
      from: `inline bool eqgt(double x, double y) { return round_float(x - y) >= 0; }`,
      to: `inline bool eqgt(double x, double y) { return round_float(x - y) > 0; }`,
    },
    {
      note: "eqlt：用 < 0",
      file: "native/lfw/utils/math/float_equal.h",
      from: `inline bool eqlt(double x, double y) { return round_float(x - y) <= 0; }`,
      to: `inline bool eqlt(double x, double y) { return round_float(x - y) < 0; }`,
    },
    {
      note: "eqgt：差值算反",
      file: "native/lfw/utils/math/float_equal.h",
      from: `inline bool eqgt(double x, double y) { return round_float(x - y) >= 0; }`,
      to: `inline bool eqgt(double x, double y) { return round_float(y - x) >= 0; }`,
    },
    {
      note: "round_float：除以倍数写成乘",
      file: "native/lfw/utils/math/round_float.h",
      from: `  return round(n * multiplier) / multiplier;`,
      to: `  return round(n * multiplier) * multiplier;`,
    },
    {
      note: "round_float：默认倍数写成 100",
      file: "native/lfw/utils/math/round_float.h",
      from: `inline double round_float(double n, double multiplier = 1000.0) {`,
      to: `inline double round_float(double n, double multiplier = 100.0) {`,
    },
    {
      note: "round_float：用 floor 而不是 round",
      file: "native/lfw/utils/math/round_float.h",
      from: `  return round(n * multiplier) / multiplier;`,
      to: `  return floor(n * multiplier) / multiplier;`,
    },
    {
      note: "floor_float：用 round 而不是 floor",
      file: "native/lfw/utils/math/floor_float.h",
      from: `  return floor(n * multiplier) / multiplier;`,
      to: `  return round(n * multiplier) / multiplier;`,
    },

    {
      note: "range：死循环判定用 <= 0",
      file: "native/lfw/utils/math/range.cpp",
      from: `  if (gap == 0.0 || (to - from) / gap < 0.0) return std::nullopt;`,
      to: `  if (gap == 0.0 || (to - from) / gap <= 0.0) return std::nullopt;`,
    },
    {
      note: "range：首元素推入 to",
      file: "native/lfw/utils/math/range.cpp",
      from: `  ret.push_back(from);`,
      to: `  ret.push_back(to);`,
    },
    {
      note: "range：循环从 i = 0 开始",
      file: "native/lfw/utils/math/range.cpp",
      from: `  for (double i = 1.0;; i += 1.0) {`,
      to: `  for (double i = 0.0;; i += 1.0) {`,
    },
    {
      note: "range：步进翻倍",
      file: "native/lfw/utils/math/range.cpp",
      from: `  for (double i = 1.0;; i += 1.0) {`,
      to: `  for (double i = 1.0;; i += 2.0) {`,
    },
    {
      note: "range：忘乘 gap",
      file: "native/lfw/utils/math/range.cpp",
      from: `    const double v = from + i * gap;`,
      to: `    const double v = from + i;`,
    },
    {
      note: "range：正向终止条件用 >=",
      file: "native/lfw/utils/math/range.cpp",
      from: `    if (gap > 0.0 ? v > to : v < to) break;`,
      to: `    if (gap > 0.0 ? v >= to : v < to) break;`,
    },
    {
      note: "range：正负分支的判断互换",
      file: "native/lfw/utils/math/range.cpp",
      from: `    if (gap > 0.0 ? v > to : v < to) break;`,
      to: `    if (gap > 0.0 ? v < to : v > to) break;`,
    },

    {
      note: "probability：clamp 的上下界互换",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  const double x = clamp(p, 0.0, 1.0);`,
      to: `  const double x = clamp(p, 1.0, 0.0);`,
    },
    {
      note: "probability：clamp 上界收成 0.5",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  const double x = clamp(p, 0.0, 1.0);`,
      to: `  const double x = clamp(p, 0.0, 0.5);`,
    },
    {
      note: "probability：x <= 0 直接返回 1",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  if (x <= 0.0) return 0.0;`,
      to: `  if (x <= 0.0) return 1.0;`,
    },
    {
      note: "probability：x >= 1 直接返回 0",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  if (x >= 1.0) return 1.0;`,
      to: `  if (x >= 1.0) return 0.0;`,
    },
    {
      note: "probability：指数写成 times",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  return 1.0 - pow(1.0 - x, 1.0 / times);`,
      to: `  return 1.0 - pow(1.0 - x, times);`,
    },
    {
      note: "probability：忘掉 1 - 的部分",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  return 1.0 - pow(1.0 - x, 1.0 / times);`,
      to: `  return pow(1.0 - x, 1.0 / times);`,
    },
    {
      note: "probability：底数写成 1 + x",
      file: "native/lfw/utils/math/probability.cpp",
      from: `  return 1.0 - pow(1.0 - x, 1.0 / times);`,
      to: `  return 1.0 - pow(1.0 + x, 1.0 / times);`,
    },

    {
      note: "normalize_plane：只翻转 a/b/c 不翻转 d",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `    a = -a;
    b = -b;
    c = -c;
    d = -d;`,
      to: `    a = -a;
    b = -b;
    c = -c;`,
    },
    {
      note: "normalize_plane：符号判定漏掉第三段",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `  if (a < 0.0 || (a == 0.0 && b < 0.0) || (a == 0.0 && b == 0.0 && c < 0.0)) {`,
      to: `  if (a < 0.0 || (a == 0.0 && b < 0.0)) {`,
    },
    {
      note: "normalize_plane：符号判定用 <= 0",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `  if (a < 0.0 || (a == 0.0 && b < 0.0) || (a == 0.0 && b == 0.0 && c < 0.0)) {`,
      to: `  if (a <= 0.0 || (a == 0.0 && b < 0.0) || (a == 0.0 && b == 0.0 && c < 0.0)) {`,
    },
    {
      note: "normalize_plane：任一坐标为负就翻转",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `  if (a < 0.0 || (a == 0.0 && b < 0.0) || (a == 0.0 && b == 0.0 && c < 0.0)) {`,
      to: `  if (a < 0.0 || b < 0.0 || c < 0.0) {`,
    },
    {
      note: "normalize_plane：d 写成 -d",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `  g_result.d = d;`,
      to: `  g_result.d = -d;`,
    },
    {
      note: "normalize_plane：c 写进 b",
      file: "native/lfw/utils/math/normalize_plane.cpp",
      from: `  g_result.b = b;
  g_result.c = c;`,
      to: `  g_result.b = b;
  g_result.c = b;`,
    },

    {
      note: "calc_plane：a 分量符号取反",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  const double a = v1_y * v2_z - v1_z * v2_y;`,
      to: `  const double a = v1_z * v2_y - v1_y * v2_z;`,
    },
    {
      note: "calc_plane：b 分量符号取反",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  const double b = v1_z * v2_x - v1_x * v2_z;`,
      to: `  const double b = v1_x * v2_z - v1_z * v2_x;`,
    },
    {
      note: "calc_plane：c 分量符号取反",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  const double c = v1_x * v2_y - v1_y * v2_x;`,
      to: `  const double c = v1_y * v2_x - v1_x * v2_y;`,
    },
    {
      note: "calc_plane：退化判定从与变成或",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  if (abs(a) < eps && abs(b) < eps && abs(c) < eps) return nullptr;`,
      to: `  if (abs(a) < eps || abs(b) < eps || abs(c) < eps) return nullptr;`,
    },
    {
      note: "calc_plane：退化判定永不成立",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  if (abs(a) < eps && abs(b) < eps && abs(c) < eps) return nullptr;`,
      to: `  if (false) return nullptr;`,
    },
    {
      note: "calc_plane：d 的符号写反",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  g_result.d = -a * x1 - b * y1 - c * z1;`,
      to: `  g_result.d = a * x1 + b * y1 + c * z1;`,
    },
    {
      note: "calc_plane：v1 的方向算反",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  const double v1_x = x2 - x1;`,
      to: `  const double v1_x = x1 - x2;`,
    },
    {
      note: "calc_plane：a 存成 -a",
      file: "native/lfw/utils/math/calc_plane.cpp",
      from: `  g_result.a = a;`,
      to: `  g_result.a = -a;`,
    },

    {
      note: "line_plane：参数式方向取成起点",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  if (is_direction) {
    vx = x2;
    vy = y2;
    vz = z2;`,
      to: `  if (is_direction) {
    vx = x1;
    vy = y2;
    vz = z2;`,
    },
    {
      note: "line_plane：方向算反",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `    vx = x2 - x1;
    vy = y2 - y1;
    vz = z2 - z1;`,
      to: `    vx = x1 - x2;
    vy = y2 - y1;
    vz = z2 - z1;`,
    },
    {
      note: "line_plane：denom 少一项且符号取反",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  const double denom = a * vx + b * vy + c * vz;`,
      to: `  const double denom = a * vx + b * vy - c * vz;`,
    },
    {
      note: "line_plane：平行判定永不成立",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  if (abs(denom) < eps) return nullptr;`,
      to: `  if (false) return nullptr;`,
    },
    {
      note: "line_plane：t 的分子漏掉 d",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  const double t = -(a * x1 + b * y1 + c * z1 + d) / denom;`,
      to: `  const double t = -(a * x1 + b * y1 + c * z1) / denom;`,
    },
    {
      note: "line_plane：线段判定忽略 is_segment",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  if (is_segment && (t < -eps || t > 1.0 + eps)) return nullptr;`,
      to: `  if (t < -eps || t > 1.0 + eps) return nullptr;`,
    },
    {
      note: "line_plane：线段上界忽略 eps",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  if (is_segment && (t < -eps || t > 1.0 + eps)) return nullptr;`,
      to: `  if (is_segment && (t < -eps || t > 1.0)) return nullptr;`,
    },
    {
      note: "line_plane：交点的 y 用 x 的表达式",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  g_result.x = x1 + t * vx;`,
      to: `  g_result.x = x1 + t * vy;`,
    },
    {
      note: "line_plane：交点的 z 用 y 的方向",
      file: "native/lfw/utils/math/line_plane_intersection.cpp",
      from: `  g_result.z = z1 + t * vz;`,
      to: `  g_result.z = z1 + t * vy;`,
    },

    {
      note: "project_to_line：分母用三次方",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  const double d = round_float(pow(m, 2.0) + pow(n, 2.0));`,
      to: `  const double d = round_float(pow(m, 3.0) + pow(n, 2.0));`,
    },
    {
      note: "project_to_line：分母第二项写成减",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  const double d = round_float(pow(m, 2.0) + pow(n, 2.0));`,
      to: `  const double d = round_float(pow(m, 2.0) - pow(n, 2.0));`,
    },
    {
      note: "project_to_line：退化判定永不成立",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  if (d == 0.0) return std::nullopt;`,
      to: `  if (false) return std::nullopt;`,
    },
    {
      note: "project_to_line：投影系数用 m 而不是 n",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  const double t = ((x - 0.0) * m + (y - 0.0) * n) / d;`,
      to: `  const double t = ((x - 0.0) * m + (y - 0.0) * m) / d;`,
    },
    {
      note: "project_to_line：除以 d 写成乘",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  const double t = ((x - 0.0) * m + (y - 0.0) * n) / d;`,
      to: `  const double t = ((x - 0.0) * m + (y - 0.0) * n) * d;`,
    },
    {
      note: "project_to_line：两个分量返回值互换",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  return std::array<double, 2>{round_float(t * m), round_float(t * n)};`,
      to: `  return std::array<double, 2>{round_float(t * n), round_float(t * m)};`,
    },
    {
      note: "project_to_line：第一个分量不做量化",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  return std::array<double, 2>{round_float(t * m), round_float(t * n)};`,
      to: `  return std::array<double, 2>{t * m, round_float(t * n)};`,
    },
    {
      note: "project_to_line：第二个分量用 m",
      file: "native/lfw/utils/math/project_to_line.cpp",
      from: `  return std::array<double, 2>{round_float(t * m), round_float(t * n)};`,
      to: `  return std::array<double, 2>{round_float(t * m), round_float(t * m)};`,
    },
  ],
};
