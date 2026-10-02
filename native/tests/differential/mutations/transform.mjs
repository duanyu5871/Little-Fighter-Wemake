/**
 * transform 变异规格
 *
 * 覆盖 subject: transform —— `native/lfw/transform.{h,cpp}`（TS 源 `src/LFW/Transform.ts`）。
 * 全部变异都在 .cpp 上（约 2.5s/条），用例 3 个文件（basic / tween / edge）。
 *
 * 记录：接口简化（不算偏差，语义等价）
 *  1. TS `move_to(x, y, z, opts: ITransformTweenOpts = {})` 里 `opts.rate ?? 0.1` ⇒
 *     C++ 用 `std::optional<double> rate` + `rate.value_or(0.1)`。两侧"未给值"与"给了值"同语义，
 *     但 harness 必须做翻译：TS 侧要包成 `{ rate }` 对象（`num(tok, 4)` 直接当 opts 传会让
 *     `opts.rate` 变 undefined ⇒ 静默回落 0.1，本轮差分就是这么抓到第一次 drift 的）。
 *  2. TS 的属性 accessor（`get x()` / `set x(v)`）与同名方法（`set_rotation(...)`）在 C++ 里无法重载，
 *     故属性 setter 记为 `set_x` / `set_y` / `set_z` / `set_scale_x|y|z` / `set_rotation_value`，
 *     方法保持原名 `set_position` / `set_scale` / `set_rotation` / `move_to` / `scale_to` / `rotate_to`。
 *  3. `wrap_angle` 在 TS 里是模块私有 ⇒ C++ 放成 `Transform` 的私有静态函数，harness 只通过
 *     `rotation` 两个入口间接观测（`rx` = 属性、`rot` = 方法），避免造出 TS 侧取不到的入口。
 *
 * 未注入的变异（理由）：
 *  1. `wrap` 里 `a % (Math.PI * 2)` —— TS 的 `%` 与 C++ `std::fmod` 对负数/无穷同语义（都取截断余数、
 *     inf ⇒ NaN），把 `std::fmod` 换成别的求余写法（如 `a - trunc(a / 2π) * 2π`）在 |a| 较小时数学等价，
 *     但会引入精度差异，属于"换实现"而非"注入漂移"，不写。
 *  2. `TransformData` 的成员初值（`x = 0 / scale_x = 1`）—— 构造后主值 `_x`/`_scale_x` 也各自有初值，
 *     两个字段在 `new` 后总是同时被写，改成员初值只影响 `d()` 未被 `set_*` 前的读法 ⇒ 可观测，
 *     已由 `new` + `snap` 覆盖；但"只改 `TransformData` 初值而主值不变"会立刻被 snap 抓到，
 *     属于重复覆盖同一分支，故只在主值/目标值各留一条（见下面 12/13）。
 */
export default {
  subject: "transform",
  mutations: [
    // ---- wrap（角度归一） ----
    {
      note: "wrap: 周期写成 PI（半个周期）",
      file: "native/lfw/transform.cpp",
      from: "  double r = std::fmod(a, PI * 2.0);",
      to: "  double r = std::fmod(a, PI);",
    },
    {
      note: "wrap: 上界改成闭区间",
      file: "native/lfw/transform.cpp",
      from: "  if (r > PI) r -= PI * 2.0;",
      to: "  if (r >= PI) r -= PI * 2.0;",
    },
    {
      note: "wrap: 上界修正方向写反",
      file: "native/lfw/transform.cpp",
      from: "  if (r > PI) r -= PI * 2.0;",
      to: "  if (r > PI) r += PI * 2.0;",
    },
    {
      note: "wrap: 下界改成闭区间",
      file: "native/lfw/transform.cpp",
      from: "  else if (r < -PI) r += PI * 2.0;",
      to: "  else if (r <= -PI) r += PI * 2.0;",
    },
    {
      note: "wrap: 下界修正方向写反",
      file: "native/lfw/transform.cpp",
      from: "  else if (r < -PI) r += PI * 2.0;",
      to: "  else if (r < -PI) r -= PI * 2.0;",
    },

    // ---- 属性 setter：主值与 _d 必须同时写 ----
    {
      note: "set_x: 只写主值不写目标",
      file: "native/lfw/transform.cpp",
      from: "  _d.x = _x = x.value_or(_x);",
      to: "  _x = x.value_or(_x);",
    },
    {
      note: "set_y: 只写主值不写目标",
      file: "native/lfw/transform.cpp",
      from: "  _d.y = _y = y.value_or(_y);",
      to: "  _y = y.value_or(_y);",
    },
    {
      note: "rotation 属性 setter: 不归一角度",
      file: "native/lfw/transform.h",
      from: "  void set_rotation_value(double v) { _d.rotation = _rotation = wrap(v); }",
      to: "  void set_rotation_value(double v) { _d.rotation = _rotation = v; }",
    },
    {
      note: "scale_x 属性 setter: 只写主值不写目标",
      file: "native/lfw/transform.h",
      from: "  void set_scale_x(double v) { _d.scale_x = _scale_x = v; }",
      to: "  void set_scale_x(double v) { _scale_x = v; }",
    },

    // ---- update 的收敛公式 ----
    {
      note: "update: dt 默认值改成 0",
      file: "native/lfw/transform.cpp",
      from: "  const double step = dt.value_or(1.0);",
      to: "  const double step = dt.value_or(0.0);",
    },
    {
      note: "update: 平滑开关判断取反",
      file: "native/lfw/transform.cpp",
      from: "  if (!_smoothing) return *this;",
      to: "  if (_smoothing) return *this;",
    },
    {
      note: "update: 收敛系数忘乘 dt",
      file: "native/lfw/transform.cpp",
      from: "  const double k = 1.0 - pow(1.0 - _rate, step);",
      to: "  const double k = 1.0 - pow(1.0 - _rate, 1.0);",
    },
    {
      note: "update: 收敛系数漏掉 1 -",
      file: "native/lfw/transform.cpp",
      from: "  const double k = 1.0 - pow(1.0 - _rate, step);",
      to: "  const double k = pow(1.0 - _rate, step);",
    },
    {
      note: "update: 底数直接用 rate",
      file: "native/lfw/transform.cpp",
      from: "  const double k = 1.0 - pow(1.0 - _rate, step);",
      to: "  const double k = 1.0 - pow(_rate, step);",
    },
    {
      note: "update: x 忘乘系数（直接跳到目标）",
      file: "native/lfw/transform.cpp",
      from: "  _x = round_float(_x + (target.x - _x) * k, 100.0);",
      to: "  _x = round_float(_x + (target.x - _x), 100.0);",
    },
    {
      note: "update: x 的差值方向写反",
      file: "native/lfw/transform.cpp",
      from: "  _x = round_float(_x + (target.x - _x) * k, 100.0);",
      to: "  _x = round_float(_x + (_x - target.x) * k, 100.0);",
    },
    {
      note: "update: x 的量化精度写成默认 1000",
      file: "native/lfw/transform.cpp",
      from: "  _x = round_float(_x + (target.x - _x) * k, 100.0);",
      to: "  _x = round_float(_x + (target.x - _x) * k, 1000.0);",
    },
    {
      note: "update: y 用 x 的差值",
      file: "native/lfw/transform.cpp",
      from: "  _y = round_float(_y + (target.y - _y) * k, 100.0);",
      to: "  _y = round_float(_y + (target.x - _y) * k, 100.0);",
    },
    {
      note: "update: z 忘了量化",
      file: "native/lfw/transform.cpp",
      from: "  _z = round_float(_z + (target.z - _z) * k, 100.0);",
      to: "  _z = _z + (target.z - _z) * k;",
    },
    {
      note: "update: 旋转不取短弧（去掉内层 wrap）",
      file: "native/lfw/transform.cpp",
      from: "  _rotation = wrap(_rotation + wrap(target.rotation - _rotation) * k);",
      to: "  _rotation = wrap(_rotation + (target.rotation - _rotation) * k);",
    },
    {
      note: "update: 旋转不归一（去掉外层 wrap）",
      file: "native/lfw/transform.cpp",
      from: "  _rotation = wrap(_rotation + wrap(target.rotation - _rotation) * k);",
      to: "  _rotation = _rotation + wrap(target.rotation - _rotation) * k;",
    },
    {
      note: "update: 缩放量化精度写成 100",
      file: "native/lfw/transform.cpp",
      from: "  _scale_x = round_float(_scale_x + (target.scale_x - _scale_x) * k, 10000.0);",
      to: "  _scale_x = round_float(_scale_x + (target.scale_x - _scale_x) * k, 100.0);",
    },
    {
      note: "update: scale_y 用 scale_x 的目标",
      file: "native/lfw/transform.cpp",
      from: "  _scale_y = round_float(_scale_y + (target.scale_y - _scale_y) * k, 10000.0);",
      to: "  _scale_y = round_float(_scale_y + (target.scale_x - _scale_y) * k, 10000.0);",
    },
    {
      note: "update: 到达守卫取反",
      file: "native/lfw/transform.cpp",
      from: "  if (is_arrived()) {",
      to: "  if (!is_arrived()) {",
    },
    {
      note: "update: 到达时不吸附到目标值",
      file: "native/lfw/transform.cpp",
      from: "  if (is_arrived()) {\n    _x = target.x;\n    _y = target.y;\n    _z = target.z;\n    _rotation = target.rotation;\n    _scale_x = target.scale_x;\n    _scale_y = target.scale_y;\n    _scale_z = target.scale_z;\n    _smoothing = false;\n  }",
      to: "  if (is_arrived()) {\n    _smoothing = false;\n  }",
    },
    {
      note: "update: 到达后 smoothing 仍置真",
      file: "native/lfw/transform.cpp",
      from: "    _smoothing = false;\n  }\n  return *this;",
      to: "    _smoothing = true;\n  }\n  return *this;",
    },

    // ---- set_position / set_scale / set_rotation ----
    {
      note: "set_position: 缺参缺口写成 0",
      file: "native/lfw/transform.cpp",
      from: "  _d.x = _x = x.value_or(_x);",
      to: "  _d.x = _x = x.value_or(0.0);",
    },
    {
      note: "set_position: 不清 smoothing",
      file: "native/lfw/transform.cpp",
      from: "  _d.z = _z = z.value_or(_z);\n  _smoothing = false;\n  return *this;\n}\n\nTransform& Transform::set_scale",
      to: "  _d.z = _z = z.value_or(_z);\n  return *this;\n}\n\nTransform& Transform::set_scale",
    },
    {
      note: "set_scale: 缺参缺口写成 1",
      file: "native/lfw/transform.cpp",
      from: "  _d.scale_x = _scale_x = x.value_or(_scale_x);",
      to: "  _d.scale_x = _scale_x = x.value_or(1.0);",
    },
    {
      note: "set_scale: 不清 smoothing",
      file: "native/lfw/transform.cpp",
      from: "  _d.scale_z = _scale_z = z.value_or(_scale_z);\n  _smoothing = false;",
      to: "  _d.scale_z = _scale_z = z.value_or(_scale_z);",
    },
    {
      note: "set_rotation: z 分量写到 y 上",
      file: "native/lfw/transform.cpp",
      from: "  _d.scale_z = _scale_z = z.value_or(_scale_z);",
      to: "  _d.scale_y = _scale_y = z.value_or(_scale_z);",
    },
    {
      note: "set_rotation: 缺参缺口写成 0",
      file: "native/lfw/transform.cpp",
      from: "  _d.rotation = _rotation = wrap(rotation.value_or(_rotation));",
      to: "  _d.rotation = _rotation = wrap(rotation.value_or(0.0));",
    },
    {
      note: "set_rotation: 不清 smoothing",
      file: "native/lfw/transform.cpp",
      from: "  _d.rotation = _rotation = wrap(rotation.value_or(_rotation));\n  _smoothing = false;",
      to: "  _d.rotation = _rotation = wrap(rotation.value_or(_rotation));",
    },

    // ---- move_to / scale_to / rotate_to ----
    {
      note: "move_to: rate 缺省写成 1",
      file: "native/lfw/transform.cpp",
      from: "  _d.z = z.value_or(_z);\n  _rate = rate.value_or(0.1);\n  _smoothing = true;",
      to: "  _d.z = z.value_or(_z);\n  _rate = rate.value_or(1.0);\n  _smoothing = true;",
    },
    {
      note: "move_to: x 缺参缺口写成 0",
      file: "native/lfw/transform.cpp",
      from: "  _d.x = x.value_or(_x);\n  _d.y = y.value_or(_y);",
      to: "  _d.x = x.value_or(0.0);\n  _d.y = y.value_or(_y);",
    },
    {
      note: "rotate_to: 目标角不归一",
      file: "native/lfw/transform.cpp",
      from: "  _d.rotation = wrap(rotation.value_or(_rotation));\n  _rate = rate.value_or(0.1);",
      to: "  _d.rotation = rotation.value_or(_rotation);\n  _rate = rate.value_or(0.1);",
    },
    {
      note: "scale_to: z 目标写到 y 上",
      file: "native/lfw/transform.cpp",
      from: "  _d.scale_z = z.value_or(_scale_z);\n  _rate = rate.value_or(0.1);",
      to: "  _d.scale_y = z.value_or(_scale_z);\n  _rate = rate.value_or(0.1);",
    },

    // ---- is_arrived ----
    {
      note: "is_arrived: eps 缺省写成 0",
      file: "native/lfw/transform.cpp",
      from: "  const double e = eps.value_or(0.01);",
      to: "  const double e = eps.value_or(0.0);",
    },
    {
      note: "is_arrived: 七个分量改或运算",
      file: "native/lfw/transform.cpp",
      from: "  return std::abs(_x - _d.x) < e && std::abs(_y - _d.y) < e && std::abs(_z - _d.z) < e &&",
      to: "  return std::abs(_x - _d.x) < e || std::abs(_y - _d.y) < e || std::abs(_z - _d.z) < e ||",
    },
    {
      note: "is_arrived: 旋转忘了短弧包裹",
      file: "native/lfw/transform.cpp",
      from: "         std::abs(wrap(_rotation - _d.rotation)) < e && std::abs(_scale_x - _d.scale_x) < e &&",
      to: "         std::abs(_rotation - _d.rotation) < e && std::abs(_scale_x - _d.scale_x) < e &&",
    },
    {
      note: "is_arrived: 边界改成闭区间",
      file: "native/lfw/transform.cpp",
      from: "         std::abs(_scale_y - _d.scale_y) < e && std::abs(_scale_z - _d.scale_z) < e;",
      to: "         std::abs(_scale_y - _d.scale_y) <= e && std::abs(_scale_z - _d.scale_z) < e;",
    },
    {
      note: "is_arrived: 漏掉 scale_x 分量",
      file: "native/lfw/transform.cpp",
      from: "         std::abs(wrap(_rotation - _d.rotation)) < e && std::abs(_scale_x - _d.scale_x) < e &&\n         std::abs(_scale_y - _d.scale_y) < e && std::abs(_scale_z - _d.scale_z) < e;",
      to: "         std::abs(wrap(_rotation - _d.rotation)) < e &&\n         std::abs(_scale_y - _d.scale_y) < e && std::abs(_scale_z - _d.scale_z) < e;",
    },

    // ---- snapshot ----
    {
      note: "snapshot: x 取目标值而不是主值",
      file: "native/lfw/transform.cpp",
      from: "  out.x = _x;",
      to: "  out.x = _d.x;",
    },
    {
      note: "snapshot: rotation 取目标值",
      file: "native/lfw/transform.cpp",
      from: "  out.rotation = _rotation;",
      to: "  out.rotation = _d.rotation;",
    },
    {
      note: "snapshot: scale_z 取目标值",
      file: "native/lfw/transform.cpp",
      from: "  out.scale_z = _scale_z;",
      to: "  out.scale_z = _d.scale_z;",
    },
  ],
};
