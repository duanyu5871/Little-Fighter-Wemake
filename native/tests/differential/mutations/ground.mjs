/**
 * ground 变异规格
 *
 * 覆盖 subject: ground —— `native/lfw/ground.{h,cpp}`（TS 源 `src/LFW/Ground.ts`，467 行）。
 * 用例 2 个文件（terrain 112 行 / edge 83 行）。
 *
 * 接口简化（语义等价，已记录）：
 *  1. TS `Ground` 构造时吃 `World`，每次调用现读 `world.bg.data.terrain` ⇒ C++ 改成
 *     `set_terrain(const std::vector<ITerrainInfo>*)` 注入指针，方法内**每次现读**（保留"地形可被整体替换"
 *     的语义），harness 侧 TS 用 `{ bg: { data: { terrain } } }` 桩对象、C++ 用同一个 vector 的地址。
 *  2. `block()` 在 TS 里返回**复用**的 `_ret` 数组（同一对象），C++ 也复用成员 vector（返回 const 引用）✓
 *     `intersect()` 的 `_intersectResult` 同理。
 *  3. `enterable()` 的 `number | null` ⇒ `std::optional<double>`；`intersect_wall()` 的 `{x,z} | null`
 *     ⇒ `std::optional<BlockPoint>`。
 *  4. `ITerrainInfo` 的 `id?`/`name?`（可缺）⇒ C++ `std::u16string`（缺省为空串），打印时两侧都映射成 `-`。
 *
 * 未注入的变异（理由）：
 *  1. `intersect()` 里 `denom == 0.0 ? 0.0 : …` 的守卫：`denom = dx²+dy²+dz²` 为 0 ⇔ 射线零长 ⇔
 *     `line_plane_intersection` 的 `a*vx+b*vy+c*vz` 也为 0 ⇒ 在那之前已经 `return nullptr` ⇒ 不可达。
 *  2. `is_direction=false, is_segment=true` 两个实参（TS 侧同样写死）：改成别的组合会撞上
 *     `line_plane_intersection` 的越界剪裁逻辑，属"换实现"而非漂移；该函数本身由 math 轮的规格覆盖。
 *  3. `_step`（=10）改成别的值确实可观测，但它与 `enterable`/`block`/`intersect` 的 `> _step` 判断
 *     是同一处，改 `_step` 等价于把那 6 处一起改；本轮选择"逐处改比较符"（见下），避免重复覆盖同一分支。
 *  4. `intersect_wall()` 的快速跳过 `if (max_h - min_y <= _step) continue;` 改成 `<` —— **恒等**：
 *     进入这里时若 `max_h - min_y == _step`，则对任意交点都有
 *     `ty <= max_h`（地形高度 h() 把 t 夹在 [0,1]，值必在 [h1,h2] 内）且 `iy >= min_y`
 *     （交点是两端的凸组合）⇒ `ty - iy <= max_h - min_y == _step` ⇒ 后面四条墙面的
 *     `ty - iy > _step` 全都不可能成立 ⇒ 跳不跳过结果一样。
 */
export default {
  subject: "ground",
  mutations: [
    // ---- 常量表（horizon / abyss） ----
    {
      note: "horizon: id 写错",
      file: "native/lfw/ground.cpp",
      from: "    r.id = u\"horizon_0\";",
      to: "    r.id = u\"horizon_1\";",
    },
    {
      note: "horizon: h1 改成 1",
      file: "native/lfw/ground.cpp",
      from: "    r.h1 = 0.0;",
      to: "    r.h1 = 1.0;",
    },
    {
      note: "abyss: h1 改成 0",
      file: "native/lfw/ground.cpp",
      from: "    r.h1 = -9007199254740991.0;",
      to: "    r.h1 = 0.0;",
    },
    {
      note: "abyss: h2 改成 0",
      file: "native/lfw/ground.cpp",
      from: "    r.h2 = -9007199254740991.0;",
      to: "    r.h2 = 0.0;",
    },
    {
      note: "abyss: id 写错",
      file: "native/lfw/ground.cpp",
      from: "    r.id = u\"ABYSS_0\";",
      to: "    r.id = u\"ABYSS_1\";",
    },
    {
      note: "horizon: x2 收窄 1",
      file: "native/lfw/ground.cpp",
      from: "    r.id = u\"horizon_0\";\n    r.name = u\"horizon_0\";\n    r.type = static_cast<int>(TerrainEnum::Flat);\n    r.x1 = -9007199254740991.0;\n    r.x2 = 9007199254740991.0;",
      to: "    r.id = u\"horizon_0\";\n    r.name = u\"horizon_0\";\n    r.type = static_cast<int>(TerrainEnum::Flat);\n    r.x1 = -9007199254740991.0;\n    r.x2 = 9007199254740990.0;",
    },

    // ---- segment ----
    {
      note: "segment: x 上界改成闭区间（跳过边界段）",
      file: "native/lfw/ground.cpp",
      from: "      if (x < seg.x1 || x > seg.x2) continue;",
      to: "      if (x < seg.x1 || x >= seg.x2) continue;",
    },
    {
      note: "segment: x 下界改成开区间",
      file: "native/lfw/ground.cpp",
      from: "      if (x < seg.x1 || x > seg.x2) continue;",
      to: "      if (x <= seg.x1 || x > seg.x2) continue;",
    },
    {
      note: "segment: z 上界改成闭区间",
      file: "native/lfw/ground.cpp",
      from: "      if (z < seg.z1 || z > seg.z2) continue;",
      to: "      if (z < seg.z1 || z >= seg.z2) continue;",
    },
    {
      note: "segment: z 下界改成开区间",
      file: "native/lfw/ground.cpp",
      from: "      if (z < seg.z1 || z > seg.z2) continue;",
      to: "      if (z <= seg.z1 || z > seg.z2) continue;",
    },
    {
      note: "segment: 等高时取最后一个（> 改 >=）",
      file: "native/lfw/ground.cpp",
      from: "      if (!has_best || seg_y > best_y) {",
      to: "      if (!has_best || seg_y >= best_y) {",
    },
    {
      note: "segment: 命中也不返回，恒回落 base",
      file: "native/lfw/ground.cpp",
      from: "    if (best != nullptr) return *best;",
      to: "    if (best != nullptr) return base();",
    },
    {
      note: "segment: 回落改用 abyss",
      file: "native/lfw/ground.cpp",
      from: "  return base();\n}",
      to: "  return abyss();\n}",
    },

    // ---- y ----
    {
      note: "y: Flat 取 h2",
      file: "native/lfw/ground.cpp",
      from: "      return seg.h1;",
      to: "      return seg.h2;",
    },
    {
      note: "y: SlopeH 不 clamp x",
      file: "native/lfw/ground.cpp",
      from: "      x = clamp(x, seg.x1, seg.x2);\n      z = clamp(z, seg.z1, seg.z2);\n      const double t = (x - seg.x1) / (seg.x2 - seg.x1);",
      to: "      z = clamp(z, seg.z1, seg.z2);\n      const double t = (x - seg.x1) / (seg.x2 - seg.x1);",
    },
    {
      note: "y: SlopeH 的 t 用 z 算",
      file: "native/lfw/ground.cpp",
      from: "      const double t = (x - seg.x1) / (seg.x2 - seg.x1);",
      to: "      const double t = (z - seg.z1) / (seg.z2 - seg.z1);",
    },
    {
      note: "y: SlopeH 插值方向写反",
      file: "native/lfw/ground.cpp",
      from: "      const double t = (x - seg.x1) / (seg.x2 - seg.x1);\n      return seg.h1 + t * (seg.h2 - seg.h1);",
      to: "      const double t = (x - seg.x1) / (seg.x2 - seg.x1);\n      return seg.h2 + t * (seg.h1 - seg.h2);",
    },
    {
      note: "y: SlopeV 的 t 用 x 算",
      file: "native/lfw/ground.cpp",
      from: "      const double t = (z - seg.z1) / (seg.z2 - seg.z1);",
      to: "      const double t = (x - seg.x1) / (seg.x2 - seg.x1);",
    },
    {
      note: "y: SlopeV 插值方向写反",
      file: "native/lfw/ground.cpp",
      from: "      const double t = (z - seg.z1) / (seg.z2 - seg.z1);\n      return seg.h1 + t * (seg.h2 - seg.h1);",
      to: "      const double t = (z - seg.z1) / (seg.z2 - seg.z1);\n      return seg.h2 + t * (seg.h1 - seg.h2);",
    },
    {
      note: "y: 未知 type 返回 h1 而不是 0",
      file: "native/lfw/ground.cpp",
      from: "  return 0.0;",
      to: "  return seg.h1;",
    },

    // ---- enterable ----
    {
      note: "enterable: 阈值改成开区间",
      file: "native/lfw/ground.cpp",
      from: "  if (diff_h > _step) return std::nullopt;",
      to: "  if (diff_h >= _step) return std::nullopt;",
    },
    {
      note: "enterable: 高度差方向写反",
      file: "native/lfw/ground.cpp",
      from: "  const double diff_h = dist_y - y_value;",
      to: "  const double diff_h = y_value - dist_y;",
    },
    {
      note: "enterable: 返回的是传入 y",
      file: "native/lfw/ground.cpp",
      from: "  return dist_y;",
      to: "  return y_value;",
    },

    // ---- block ----
    {
      note: "block: base 早退返回 4 个点",
      file: "native/lfw/ground.cpp",
      from: "  if (seg.id == base().id) return _empty;",
      to: "  if (seg.id == base().id) return _ret;",
    },
    {
      note: "block: l 边界内缩方向写反",
      file: "native/lfw/ground.cpp",
      from: "  double l = seg.x1 - 1.0;",
      to: "  double l = seg.x1 + 1.0;",
    },
    {
      note: "block: r 边界内缩方向写反",
      file: "native/lfw/ground.cpp",
      from: "  double r = seg.x2 + 1.0;",
      to: "  double r = seg.x2 - 1.0;",
    },
    {
      note: "block: f 边界内缩方向写反",
      file: "native/lfw/ground.cpp",
      from: "  double f = seg.z1 - 1.0;",
      to: "  double f = seg.z1 + 1.0;",
    },
    {
      note: "block: n 边界内缩方向写反",
      file: "native/lfw/ground.cpp",
      from: "  double n = seg.z2 + 1.0;",
      to: "  double n = seg.z2 - 1.0;",
    },
    {
      note: "block: mid_x 写成差的一半",
      file: "native/lfw/ground.cpp",
      from: "  const double mid_x = (seg.x1 + seg.x2) / 2.0;",
      to: "  const double mid_x = (seg.x1 - seg.x2) / 2.0;",
    },
    {
      note: "block: mid_z 写成差的一半",
      file: "native/lfw/ground.cpp",
      from: "  const double mid_z = (seg.z1 + seg.z2) / 2.0;",
      to: "  const double mid_z = (seg.z1 - seg.z2) / 2.0;",
    },
    {
      note: "block: from_l 判据改成开区间",
      file: "native/lfw/ground.cpp",
      from: "  const bool from_l = px <= (slope_x.has_value() ? *slope_x : mid_x);",
      to: "  const bool from_l = px < (slope_x.has_value() ? *slope_x : mid_x);",
    },
    {
      note: "block: from_f 判据改成开区间",
      file: "native/lfw/ground.cpp",
      from: "  const bool from_f = pz <= (slope_z.has_value() ? *slope_z : mid_z);",
      to: "  const bool from_f = pz < (slope_z.has_value() ? *slope_z : mid_z);",
    },
    {
      note: "block: SlopeH 的 clamp(t) 去掉",
      file: "native/lfw/ground.cpp",
      from: "      slope_x = seg.x1 + clamp(t, 0.0, 1.0) * (seg.x2 - seg.x1);",
      to: "      slope_x = seg.x1 + t * (seg.x2 - seg.x1);",
    },
    {
      note: "block: SlopeH 的 sx 不 clamp 到 [l,r]",
      file: "native/lfw/ground.cpp",
      from: "      const double sx = clamp(seg.x1 + t * (seg.x2 - seg.x1), l, r);",
      to: "      const double sx = seg.x1 + t * (seg.x2 - seg.x1);",
    },
    {
      note: "block: SlopeH 收边界改成 >= x",
      file: "native/lfw/ground.cpp",
      from: "      if (sx > x) r = sx + 1.0;",
      to: "      if (sx >= x) r = sx + 1.0;",
    },
    {
      note: "block: SlopeH 扩边界方向写反",
      file: "native/lfw/ground.cpp",
      from: "      else l = sx - 1.0;",
      to: "      else l = sx + 1.0;",
    },
    {
      note: "block: SlopeV 收边界改成 >= z",
      file: "native/lfw/ground.cpp",
      from: "      if (sz > z) n = sz + 1.0;",
      to: "      if (sz >= z) n = sz + 1.0;",
    },
    {
      note: "block: 近侧排序判据改闭区间",
      file: "native/lfw/ground.cpp",
      from: "  if (abs(fx - x) < abs(fz - z)) {",
      to: "  if (abs(fx - x) <= abs(fz - z)) {",
    },
    {
      note: "block: 第 0 个候选 x/z 互换",
      file: "native/lfw/ground.cpp",
      from: "    _ret[0].x = fx;\n    _ret[0].z = z;",
      to: "    _ret[0].x = z;\n    _ret[0].z = fx;",
    },
    {
      note: "block: 第 1 个候选 x/z 互换",
      file: "native/lfw/ground.cpp",
      from: "    _ret[1].x = x;\n    _ret[1].z = fz;",
      to: "    _ret[1].x = fz;\n    _ret[1].z = x;",
    },
    {
      note: "block: 远侧排序判据改闭区间",
      file: "native/lfw/ground.cpp",
      from: "  if (abs(ox - x) < abs(oz - z)) {",
      to: "  if (abs(ox - x) <= abs(oz - z)) {",
    },
    {
      note: "block: 第 3 个候选 x/z 互换",
      file: "native/lfw/ground.cpp",
      from: "    _ret[3].x = x;\n    _ret[3].z = oz;",
      to: "    _ret[3].x = oz;\n    _ret[3].z = x;",
    },

    // ---- intersect ----
    {
      note: "intersect: Flat 平面常数 d 符号写反",
      file: "native/lfw/ground.cpp",
      from: "          d = -seg.h1;",
      to: "          d = seg.h1;",
    },
    {
      note: "intersect: SlopeH 平面 a 符号写反",
      file: "native/lfw/ground.cpp",
      from: "          a = -dh;\n          b = dx2;",
      to: "          a = dh;\n          b = dx2;",
    },
    {
      note: "intersect: SlopeH 平面常数项符号写反",
      file: "native/lfw/ground.cpp",
      from: "          d = seg.x1 * dh - seg.h1 * dx2;",
      to: "          d = seg.x1 * dh + seg.h1 * dx2;",
    },
    {
      note: "intersect: SlopeV 平面常数项符号写反",
      file: "native/lfw/ground.cpp",
      from: "          d = seg.z1 * dh - seg.h1 * dz2;",
      to: "          d = seg.z1 * dh + seg.h1 * dz2;",
    },
    {
      note: "intersect: 顶面命中不校验 z 范围",
      file: "native/lfw/ground.cpp",
      from: "        if (hit->x >= seg.x1 && hit->x <= seg.x2 && hit->z >= seg.z1 && hit->z <= seg.z2) {",
      to: "        if (hit->x >= seg.x1 && hit->x <= seg.x2) {",
    },
    {
      note: "intersect: 顶面高度阈值改成开区间",
      file: "native/lfw/ground.cpp",
      from: "          if (seg_y - y1 <= _step) {",
      to: "          if (seg_y - y1 < _step) {",
    },
    {
      note: "intersect: 起点在面上不再跳过",
      file: "native/lfw/ground.cpp",
      from: "            if (t > 1e-6 && t < best_t) {",
      to: "            if (t > 0.0 && t < best_t) {",
    },
    {
      note: "intersect: 左墙推出方向写反",
      file: "native/lfw/ground.cpp",
      from: "              _hit.point.x = seg.x1 - 1.0;\n              _hit.point.y = y2;",
      to: "              _hit.point.x = seg.x1 + 1.0;\n              _hit.point.y = y2;",
    },
    {
      note: "intersect: 墙面命中 Y 用起点",
      file: "native/lfw/ground.cpp",
      from: "              _hit.point.x = seg.x1 - 1.0;\n              _hit.point.y = y2;",
      to: "              _hit.point.x = seg.x1 - 1.0;\n              _hit.point.y = y1;",
    },
    {
      note: "intersect: 无命中时分枝取反",
      file: "native/lfw/ground.cpp",
      from: "  if (!std::isfinite(best_t)) {\n    _hit.point.x = x2;",
      to: "  if (std::isfinite(best_t)) {\n    _hit.point.x = x2;",
    },
    {
      note: "intersect: 返回的地形按终点查而不是命中点",
      file: "native/lfw/ground.cpp",
      from: "  _hit.segment = segment(_hit.point.x, _hit.point.z);",
      to: "  _hit.segment = segment(x2, z2);",
    },

    // ---- intersect_wall ----
    {
      note: "wall: 零 XZ 位移判据改或",
      file: "native/lfw/ground.cpp",
      from: "  if (dx == 0.0 && dz == 0.0) return std::nullopt;",
      to: "  if (dx == 0.0 || dz == 0.0) return std::nullopt;",
    },
    {
      note: "wall: max_h 取成 h2 当 h1<h2 时错",
      file: "native/lfw/ground.cpp",
      from: "    const double max_h = seg.h1 > seg.h2 ? seg.h1 : seg.h2;",
      to: "    const double max_h = seg.h1 < seg.h2 ? seg.h1 : seg.h2;",
    },
    {
      note: "wall: 左墙推出方向写反",
      file: "native/lfw/ground.cpp",
      from: "            best_x = seg.x1 - 1.0;\n            best_z = z2;",
      to: "            best_x = seg.x1 + 1.0;\n            best_z = z2;",
    },
    {
      note: "wall: 左墙的 z 用 z1",
      file: "native/lfw/ground.cpp",
      from: "            best_x = seg.x2 + 1.0;\n            best_z = z2;",
      to: "            best_x = seg.x2 + 1.0;\n            best_z = z1;",
    },
    {
      note: "wall: 无命中判据取反",
      file: "native/lfw/ground.cpp",
      from: "  if (!std::isfinite(best_t)) return std::nullopt;",
      to: "  if (std::isfinite(best_t)) return std::nullopt;",
    },
  ],
};
