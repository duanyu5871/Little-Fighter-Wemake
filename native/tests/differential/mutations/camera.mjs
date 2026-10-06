// `Camera`（`src/LFW/Camera.ts`）+ 顺带两处新文件：`ditto/instance.h` 的 `vec2`（宿主 `Ditto` 的
// `vec2`）与 `defines/i_vector2.h` 的 `Vector2`。
//
// 用例：`cases/camera/all.txt`（`sf` 脚本化假世界 / `new` / `dump` / `jx`·`jy`·`dest`·`lock` /
// `pos`·`dset`·`vel` / `update`）。
//
// 有意不覆盖（不可观察、按构造等价或无法在台面上造出来）：
//   * `modern_screen_height()` 里 `defines::num` 找不到键时回 0 的分支：`Defines.MODERN_SCREEN_HEIGHT`
//     一定在表里（TS 侧是个模块常量，更不可能缺）；
//   * `vector2.h` 的 `set`、`i_vector2.h` 里没实现的方法：本刀没有调用点；
//   * `ditto::vec2(x, y)` 里 `x` / `y` 为 `null` / `undefined` 的情形：TS 的 three.js `Vector2`
//     会把 `null` 原样写进 `x`，端口的分量是 `double` ⇒ 记在 README 偏差表，台面只喂数字；
//   * `_dested` / `_locked` 的分量是 nullish 时的 `??` 分支：同上（TS 声明是 `number`）；
//   * `world_stage()` / `world_bg()` / `world_dataset()` 返回 nullish 时 TS 会在属性读上抛
//     `TypeError`：端口按契约要求返回对象（`field_or` 对非对象给 `undefined`），记在偏差表；
//   * harness 层的 `sf` 脚本与 `dump` / `w:` 回显行：那是台面自己的输出。
export default {
  subject: "camera",
  cases: ["all"],
  mutations: [
    // ---------------------------------------------------------------- 构造 / 复位 / 跳跃
    {
      note: "Camera: 构造时 destination 不是原点",
      file: "native/lfw/camera.cpp",
      from: `  destination = ditto::vec2();
  position = ditto::vec2();
  velocity = ditto::vec2();`,
      to: `  destination = ditto::vec2(1, 1);
  position = ditto::vec2();
  velocity = ditto::vec2();`,
    },
    {
      note: "Camera: 构造时 position 的 y 不是 0",
      file: "native/lfw/camera.cpp",
      from: `  destination = ditto::vec2();
  position = ditto::vec2();
  velocity = ditto::vec2();`,
      to: `  destination = ditto::vec2();
  position = ditto::vec2(0, 1);
  velocity = ditto::vec2();`,
    },
    {
      note: "Camera: 构造时 velocity 不是 0",
      file: "native/lfw/camera.cpp",
      from: `  destination = ditto::vec2();
  position = ditto::vec2();
  velocity = ditto::vec2();`,
      to: `  destination = ditto::vec2();
  position = ditto::vec2();
  velocity = ditto::vec2(1, 0);`,
    },
    {
      note: "Camera: reset 里 jump_x 传 1",
      file: "native/lfw/camera.cpp",
      from: `  jump_x(0);
  jump_y(0);`,
      to: `  jump_x(1);
  jump_y(0);`,
    },
    {
      note: "Camera: reset 里 jump_y 传 -1",
      file: "native/lfw/camera.cpp",
      from: `  jump_x(0);
  jump_y(0);`,
      to: `  jump_x(0);
  jump_y(-1);`,
    },
    {
      note: "Camera: reset 漏掉 jump_y",
      file: "native/lfw/camera.cpp",
      from: `  jump_x(0);
  jump_y(0);`,
      to: `  jump_x(0);`,
    },
    {
      note: "Camera: reset 不解除锁定",
      file: "native/lfw/camera.cpp",
      from: `  jump_y(0);
  _has_locked = false;
  _has_dested = false;`,
      to: `  jump_y(0);
  _has_locked = true;
  _has_dested = false;`,
    },
    {
      note: "Camera: reset 不清 dested",
      file: "native/lfw/camera.cpp",
      from: `  _has_locked = false;
  _has_dested = false;`,
      to: `  _has_locked = false;
  _has_dested = true;`,
    },
    {
      note: "Camera: jump_x 不清速度",
      file: "native/lfw/camera.cpp",
      from: `  velocity.x = 0;
  position.x = destination.x = x;`,
      to: `  position.x = destination.x = x;`,
    },
    {
      note: "Camera: jump_x 不写 destination",
      file: "native/lfw/camera.cpp",
      from: `  position.x = destination.x = x;`,
      to: `  position.x = x;`,
    },
    {
      note: "Camera: jump_y 不清速度",
      file: "native/lfw/camera.cpp",
      from: `  velocity.y = 0;
  position.y = destination.y = y;`,
      to: `  position.y = destination.y = y;`,
    },
    {
      note: "Camera: jump_y 不写 destination",
      file: "native/lfw/camera.cpp",
      from: `  position.y = destination.y = y;`,
      to: `  position.y = y;`,
    },
    {
      note: "Camera: undest 不生效",
      file: "native/lfw/camera.cpp",
      from: `void Camera::undest() { _has_dested = false; }`,
      to: `void Camera::undest() { _has_dested = true; }`,
    },
    {
      note: "Camera: dest 把 x / y 写反",
      file: "native/lfw/camera.cpp",
      from: `void Camera::dest(double x, double y) { _dested = ditto::vec2(x, y); _has_dested = true; }`,
      to: `void Camera::dest(double x, double y) { _dested = ditto::vec2(y, x); _has_dested = true; }`,
    },
    {
      note: "Camera: dest 的 y 写成 x",
      file: "native/lfw/camera.cpp",
      from: `void Camera::dest(double x, double y) { _dested = ditto::vec2(x, y); _has_dested = true; }`,
      to: `void Camera::dest(double x, double y) { _dested = ditto::vec2(x, x); _has_dested = true; }`,
    },
    {
      note: "Camera: dest 不标记 dested",
      file: "native/lfw/camera.cpp",
      from: `void Camera::dest(double x, double y) { _dested = ditto::vec2(x, y); _has_dested = true; }`,
      to: `void Camera::dest(double x, double y) { _dested = ditto::vec2(x, y); _has_dested = false; }`,
    },
    {
      note: "Camera: unlock 不生效",
      file: "native/lfw/camera.cpp",
      from: `void Camera::unlock() { _has_locked = false; }`,
      to: `void Camera::unlock() { _has_locked = true; }`,
    },
    {
      note: "Camera: lock 把 x / y 写反",
      file: "native/lfw/camera.cpp",
      from: `void Camera::lock(double x, double y) { _locked = ditto::vec2(x, y); _has_locked = true; }`,
      to: `void Camera::lock(double x, double y) { _locked = ditto::vec2(y, x); _has_locked = true; }`,
    },
    {
      note: "Camera: lock 不标记 locked",
      file: "native/lfw/camera.cpp",
      from: `void Camera::lock(double x, double y) { _locked = ditto::vec2(x, y); _has_locked = true; }`,
      to: `void Camera::lock(double x, double y) { _locked = ditto::vec2(x, y); _has_locked = false; }`,
    },

    // ---------------------------------------------------------------- update：锁定分支
    {
      note: "Camera: update 无视锁定",
      file: "native/lfw/camera.cpp",
      from: `  if (_has_locked) {
    jump_x(_locked.x);`,
      to: `  if (false) {
    jump_x(_locked.x);`,
    },
    {
      note: "Camera: update 无条件当锁定处理",
      file: "native/lfw/camera.cpp",
      from: `  if (_has_locked) {
    jump_x(_locked.x);`,
      to: `  if (true) {
    jump_x(_locked.x);`,
    },
    {
      note: "Camera: 锁定分支 jump_x 用 y",
      file: "native/lfw/camera.cpp",
      from: `    jump_x(_locked.x);
    jump_y(_locked.y);`,
      to: `    jump_x(_locked.y);
    jump_y(_locked.y);`,
    },
    {
      note: "Camera: 锁定分支 jump_y 用 x",
      file: "native/lfw/camera.cpp",
      from: `    jump_x(_locked.x);
    jump_y(_locked.y);`,
      to: `    jump_x(_locked.x);
    jump_y(_locked.x);`,
    },
    {
      note: "Camera: 锁定分支漏掉 jump_y",
      file: "native/lfw/camera.cpp",
      from: `    jump_x(_locked.x);
    jump_y(_locked.y);`,
      to: `    jump_x(_locked.x);`,
    },

    // ---------------------------------------------------------------- update：顶部三个读取
    {
      note: "Camera: 顶层 world.stage 取成 world.bg",
      file: "native/lfw/camera.cpp",
      from: `  const Value stage = _world->world_stage();
  const Value bg = _world->world_bg();`,
      to: `  const Value stage = _world->world_bg();
  const Value bg = _world->world_bg();`,
    },
    {
      note: "Camera: 顶层 world.dataset 取成 world.stage",
      file: "native/lfw/camera.cpp",
      from: `  const Value dataset = _world->world_dataset();`,
      to: `  const Value dataset = _world->world_stage();`,
    },
    {
      note: "Camera: atom_time 读成 screen_w",
      file: "native/lfw/camera.cpp",
      from: `  const double atom_time = to_number(field_or(dataset, u"atom_time"));
  const double screen_w = to_number(field_or(dataset, u"screen_w"));`,
      to: `  const double atom_time = to_number(field_or(dataset, u"screen_w"));
  const double screen_w = to_number(field_or(dataset, u"screen_w"));`,
    },
    {
      note: "Camera: screen_w 读成 screen_h",
      file: "native/lfw/camera.cpp",
      from: `  const double screen_w = to_number(field_or(dataset, u"screen_w"));
  const double screen_h = to_number(field_or(dataset, u"screen_h"));`,
      to: `  const double screen_w = to_number(field_or(dataset, u"screen_h"));
  const double screen_h = to_number(field_or(dataset, u"screen_h"));`,
    },
    {
      note: "Camera: screen_h 读成 atom_time",
      file: "native/lfw/camera.cpp",
      from: `  const double screen_h = to_number(field_or(dataset, u"screen_h"));`,
      to: `  const double screen_h = to_number(field_or(dataset, u"atom_time"));`,
    },

    // ---------------------------------------------------------------- update：x 块
    {
      note: "Camera: min_cam_x 读成 cam_r",
      file: "native/lfw/camera.cpp",
      from: `    const double min_cam_x = to_number(field_or(stage, u"cam_l"));`,
      to: `    const double min_cam_x = to_number(field_or(stage, u"cam_r"));`,
    },
    {
      note: "Camera: max_cam_x 的 cam_r 读成 cam_l",
      file: "native/lfw/camera.cpp",
      from: `        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_x"));`,
      to: `        to_number(field_or(stage, u"cam_l")) - screen_w / to_number(field_or(bg, u"zoom_x"));`,
    },
    {
      note: "Camera: max_cam_x 的 zoom_x 读成 zoom_y",
      file: "native/lfw/camera.cpp",
      from: `        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_x"));`,
      to: `        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_y"));`,
    },
    {
      note: "Camera: max_cam_x 把减改成加",
      file: "native/lfw/camera.cpp",
      from: `        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_x"));`,
      to: `        to_number(field_or(stage, u"cam_r")) + screen_w / to_number(field_or(bg, u"zoom_x"));`,
    },
    {
      note: "Camera: max_cam_x 把 zoom_x 放到分子",
      file: "native/lfw/camera.cpp",
      from: `        to_number(field_or(stage, u"cam_r")) - screen_w / to_number(field_or(bg, u"zoom_x"));`,
      to: `        to_number(field_or(stage, u"cam_r")) - to_number(field_or(bg, u"zoom_x")) / screen_w;`,
    },
    {
      note: "Camera: max_vx_ratio 改成 1",
      file: "native/lfw/camera.cpp",
      from: `    const double max_vx_ratio = 50;`,
      to: `    const double max_vx_ratio = 1;`,
    },
    {
      note: "Camera: acc_x_ratio 改成 2",
      file: "native/lfw/camera.cpp",
      from: `    const double acc_x_ratio = 1;`,
      to: `    const double acc_x_ratio = 2;`,
    },
    {
      note: "Camera: destination.x 不再用 _dested",
      file: "native/lfw/camera.cpp",
      from: `    destination.x = clamp(_has_dested ? _dested.x : destination.x, min_cam_x, max_cam_x);`,
      to: `    destination.x = clamp(_dested.x, min_cam_x, max_cam_x);`,
    },
    {
      note: "Camera: destination.x 不 clamp",
      file: "native/lfw/camera.cpp",
      from: `    destination.x = clamp(_has_dested ? _dested.x : destination.x, min_cam_x, max_cam_x);`,
      to: `    destination.x = _has_dested ? _dested.x : destination.x;`,
    },
    {
      note: "Camera: destination.x 的 clamp 上下界调换",
      file: "native/lfw/camera.cpp",
      from: `    destination.x = clamp(_has_dested ? _dested.x : destination.x, min_cam_x, max_cam_x);`,
      to: `    destination.x = clamp(_has_dested ? _dested.x : destination.x, max_cam_x, min_cam_x);`,
    },
    {
      note: "Camera: 越界判断的 < 改成 <=",
      file: "native/lfw/camera.cpp",
      from: `    if (position.x < min_cam_x || position.x > max_cam_x) {`,
      to: `    if (position.x <= min_cam_x || position.x > max_cam_x) {`,
    },
    {
      note: "Camera: 越界判断的 > 改成 >=",
      file: "native/lfw/camera.cpp",
      from: `    if (position.x < min_cam_x || position.x > max_cam_x) {`,
      to: `    if (position.x < min_cam_x || position.x >= max_cam_x) {`,
    },
    {
      note: "Camera: 越界判断的 || 改成 &&",
      file: "native/lfw/camera.cpp",
      from: `    if (position.x < min_cam_x || position.x > max_cam_x) {`,
      to: `    if (position.x < min_cam_x && position.x > max_cam_x) {`,
    },
    {
      note: "Camera: 越界分支不清速度",
      file: "native/lfw/camera.cpp",
      from: `      velocity.x = 0;
      position.x = clamp(position.x, min_cam_x, max_cam_x);`,
      to: `      position.x = clamp(position.x, min_cam_x, max_cam_x);`,
    },
    {
      note: "Camera: 越界分支的下界写成 0",
      file: "native/lfw/camera.cpp",
      from: `      position.x = clamp(position.x, min_cam_x, max_cam_x);`,
      to: `      position.x = clamp(position.x, 0, max_cam_x);`,
    },
    {
      note: "Camera: 越界分支不夹 position",
      file: "native/lfw/camera.cpp",
      from: `      position.x = clamp(position.x, min_cam_x, max_cam_x);`,
      to: `      position.x = position.x;`,
    },
    {
      note: "Camera: 对齐判断的 == 改成 !=",
      file: "native/lfw/camera.cpp",
      from: `    if (round(position.x) == round(destination.x)) break;`,
      to: `    if (round(position.x) != round(destination.x)) break;`,
    },
    {
      note: "Camera: 对齐判断两边都取 position",
      file: "native/lfw/camera.cpp",
      from: `    if (round(position.x) == round(destination.x)) break;`,
      to: `    if (round(position.x) == round(position.x)) break;`,
    },
    {
      note: "Camera: 对齐判断不取整",
      file: "native/lfw/camera.cpp",
      from: `    if (round(position.x) == round(destination.x)) break;`,
      to: `    if (position.x == destination.x) break;`,
    },
    {
      note: "Camera: acc_x 的 0.7 改成 0.8",
      file: "native/lfw/camera.cpp",
      from: `    const double acc_x = min(atom_time * acc_x_ratio,
                             atom_time * 0.7 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_w);`,
      to: `    const double acc_x = min(atom_time * acc_x_ratio,
                             atom_time * 0.8 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_w);`,
    },
    {
      note: "Camera: acc_x 的 min 改成 max",
      file: "native/lfw/camera.cpp",
      from: `    const double acc_x = min(atom_time * acc_x_ratio,`,
      to: `    const double acc_x = max(atom_time * acc_x_ratio,`,
    },
    {
      note: "Camera: acc_x 的距离用加号",
      file: "native/lfw/camera.cpp",
      from: `                             atom_time * 0.7 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_w);`,
      to: `                             atom_time * 0.7 * (acc_x_ratio * abs(position.x + destination.x)) /
                                 screen_w);`,
    },
    {
      note: "Camera: acc_x 除以 screen_h",
      file: "native/lfw/camera.cpp",
      from: `                             atom_time * 0.7 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_w);`,
      to: `                             atom_time * 0.7 * (acc_x_ratio * abs(position.x - destination.x)) /
                                 screen_h);`,
    },
    {
      note: "Camera: 方向取反（x）",
      file: "native/lfw/camera.cpp",
      from: `    const double direction_x = position.x > destination.x ? -1 : 1;`,
      to: `    const double direction_x = position.x > destination.x ? 1 : -1;`,
    },
    {
      note: "Camera: max_vx 漏掉 ratio",
      file: "native/lfw/camera.cpp",
      from: `    const double max_vx = direction_x * max_vx_ratio * acc_x;`,
      to: `    const double max_vx = direction_x * acc_x;`,
    },
    {
      note: "Camera: 方向重置判断取反（x）",
      file: "native/lfw/camera.cpp",
      from: `    if (sign(velocity.x) != direction_x) velocity.x = 0;`,
      to: `    if (sign(velocity.x) == direction_x) velocity.x = 0;`,
    },
    {
      note: "Camera: 方向重置后不清零（x）",
      file: "native/lfw/camera.cpp",
      from: `    if (sign(velocity.x) != direction_x) velocity.x = 0;`,
      to: `    if (sign(velocity.x) != direction_x) velocity.x = 1;`,
    },
    {
      note: "Camera: 加速分支用减号（x）",
      file: "native/lfw/camera.cpp",
      from: `      velocity.x += acc_x * direction_x;`,
      to: `      velocity.x -= acc_x * direction_x;`,
    },
    {
      note: "Camera: 封顶分支写成加速量（x）",
      file: "native/lfw/camera.cpp",
      from: `      velocity.x = max_vx;`,
      to: `      velocity.x = acc_x * direction_x;`,
    },
    {
      note: "Camera: 封顶比较不取绝对值（x）",
      file: "native/lfw/camera.cpp",
      from: `    if (abs(velocity.x) < abs(max_vx))`,
      to: `    if (velocity.x < max_vx)`,
    },
    {
      note: "Camera: 方向分支取反（x）",
      file: "native/lfw/camera.cpp",
      from: `    if (direction_x < 0)
      position.x = max(destination.x, position.x + velocity.x);
    else
      position.x = min(destination.x, position.x + velocity.x);`,
      to: `    if (direction_x > 0)
      position.x = max(destination.x, position.x + velocity.x);
    else
      position.x = min(destination.x, position.x + velocity.x);`,
    },
    {
      note: "Camera: 收敛用的 max / min 调换（x）",
      file: "native/lfw/camera.cpp",
      from: `    if (direction_x < 0)
      position.x = max(destination.x, position.x + velocity.x);
    else
      position.x = min(destination.x, position.x + velocity.x);`,
      to: `    if (direction_x < 0)
      position.x = min(destination.x, position.x + velocity.x);
    else
      position.x = max(destination.x, position.x + velocity.x);`,
    },
    {
      note: "Camera: 位移用减号（x）",
      file: "native/lfw/camera.cpp",
      from: `      position.x = max(destination.x, position.x + velocity.x);`,
      to: `      position.x = max(destination.x, position.x - velocity.x);`,
    },

    // ---------------------------------------------------------------- update：y 块
    {
      note: "Camera: height 读成 width 之外的同名字段（bg.height → bg.zoom_y）",
      file: "native/lfw/camera.cpp",
      from: `    const double height = to_number(field_or(bg, u"height"));`,
      to: `    const double height = to_number(field_or(bg, u"zoom_y"));`,
    },
    {
      note: "Camera: 门限的 <= 改成 <",
      file: "native/lfw/camera.cpp",
      from: `    if (height <= modern_screen_height()) {`,
      to: `    if (height < modern_screen_height()) {`,
    },
    {
      note: "Camera: 门限比较取反",
      file: "native/lfw/camera.cpp",
      from: `    if (height <= modern_screen_height()) {`,
      to: `    if (height > modern_screen_height()) {`,
    },
    {
      note: "Camera: 门限里 y 不归零（只写 destination）",
      file: "native/lfw/camera.cpp",
      from: `      position.y = destination.y = 0;`,
      to: `      destination.y = 0;`,
    },
    {
      note: "Camera: 门限里 y 写成 1",
      file: "native/lfw/camera.cpp",
      from: `      position.y = destination.y = 0;`,
      to: `      position.y = destination.y = 1;`,
    },
    {
      note: "Camera: far 从 bg 上取",
      file: "native/lfw/camera.cpp",
      from: `    const double far = to_number(field_or(_world->world_stage(), u"far"));`,
      to: `    const double far = to_number(field_or(_world->world_bg(), u"far"));`,
    },
    {
      note: "Camera: far 第二次读 world.stage 变成读 world.bg",
      file: "native/lfw/camera.cpp",
      from: `    const double far = to_number(field_or(_world->world_stage(), u"far"));`,
      to: `    _world->world_bg();
    const double far = to_number(field_or(_world->world_stage(), u"far"));`,
    },
    {
      note: "Camera: max_vy_ratio 改成 2",
      file: "native/lfw/camera.cpp",
      from: `    const double max_vy_ratio = 50;`,
      to: `    const double max_vy_ratio = 2;`,
    },
    {
      note: "Camera: acc_y_ratio 改成 0.5",
      file: "native/lfw/camera.cpp",
      from: `    const double acc_y_ratio = 1;`,
      to: `    const double acc_y_ratio = 0.5;`,
    },
    {
      note: "Camera: cam_y 不再用 _dested",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_y = _has_dested ? _dested.y : destination.y;`,
      to: `    const double cam_y = _dested.y;`,
    },
    {
      note: "Camera: cam_y 取 _dested.x",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_y = _has_dested ? _dested.y : destination.y;`,
      to: `    const double cam_y = _has_dested ? _dested.x : destination.y;`,
    },
    {
      note: "Camera: zoom_y 读成 zoom_x",
      file: "native/lfw/camera.cpp",
      from: `    const Value zoom_y_v = field_or(bg, u"zoom_y");`,
      to: `    const Value zoom_y_v = field_or(bg, u"zoom_x");`,
    },
    {
      note: "Camera: zoom_y 为 nullish 时不落回 1（落回 0）",
      file: "native/lfw/camera.cpp",
      from: `    const double zoom_y = is_nullish(zoom_y_v) ? 1 : to_number(zoom_y_v);`,
      to: `    const double zoom_y = is_nullish(zoom_y_v) ? 0 : to_number(zoom_y_v);`,
    },
    {
      note: "Camera: zoom_y 为 nullish 时不落回 1（落回 2）",
      file: "native/lfw/camera.cpp",
      from: `    const double zoom_y = is_nullish(zoom_y_v) ? 1 : to_number(zoom_y_v);`,
      to: `    const double zoom_y = is_nullish(zoom_y_v) ? 2 : to_number(zoom_y_v);`,
    },
    {
      note: "Camera: zoom_y 的 nullish 判断只看 undefined",
      file: "native/lfw/camera.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<std::monostate>(v);`,
    },
    {
      note: "Camera: cam_max_y 的 far 系数改成 -1.5",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() / zoom_y);`,
      to: `    const double cam_max_y = min(-1.5 * far, height - modern_screen_height() / zoom_y);`,
    },
    {
      note: "Camera: cam_max_y 的 min 改成 max",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() / zoom_y);`,
      to: `    const double cam_max_y = max(-0.5 * far, height - modern_screen_height() / zoom_y);`,
    },
    {
      note: "Camera: cam_max_y 里 zoom_y 放到分子",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() / zoom_y);`,
      to: `    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() * zoom_y);`,
    },
    {
      note: "Camera: cam_max_y 减去的是 height",
      file: "native/lfw/camera.cpp",
      from: `    const double cam_max_y = min(-0.5 * far, height - modern_screen_height() / zoom_y);`,
      to: `    const double cam_max_y = min(-0.5 * far, modern_screen_height() / zoom_y - height);`,
    },
    {
      note: "Camera: destination.y 的下界写成 cam_max_y",
      file: "native/lfw/camera.cpp",
      from: `    destination.y = clamp(cam_y, 0, cam_max_y);`,
      to: `    destination.y = clamp(cam_y, cam_max_y, 0);`,
    },
    {
      note: "Camera: destination.y 不 clamp",
      file: "native/lfw/camera.cpp",
      from: `    destination.y = clamp(cam_y, 0, cam_max_y);`,
      to: `    destination.y = cam_y;`,
    },
    {
      note: "Camera: acc_y 的 0.7 改成 0.6",
      file: "native/lfw/camera.cpp",
      from: `    const double acc_y = min(atom_time * acc_y_ratio,
                             atom_time * 0.7 * (acc_y_ratio * abs(position.y - destination.y)) /
                                 screen_h);`,
      to: `    const double acc_y = min(atom_time * acc_y_ratio,
                             atom_time * 0.6 * (acc_y_ratio * abs(position.y - destination.y)) /
                                 screen_h);`,
    },
    {
      note: "Camera: acc_y 的距离用加号",
      file: "native/lfw/camera.cpp",
      from: `                             atom_time * 0.7 * (acc_y_ratio * abs(position.y - destination.y)) /
                                 screen_h);`,
      to: `                             atom_time * 0.7 * (acc_y_ratio * abs(position.y + destination.y)) /
                                 screen_h);`,
    },
    {
      note: "Camera: acc_y 除以 screen_h 变成乘以 screen_h",
      file: "native/lfw/camera.cpp",
      from: `                             atom_time * 0.7 * (acc_y_ratio * abs(position.y - destination.y)) /
                                 screen_h);`,
      to: `                             atom_time * 0.7 * (acc_y_ratio * abs(position.y - destination.y)) *
                                 screen_h);`,
    },
    {
      note: "Camera: 对齐判断的 == 改成 !=（y）",
      file: "native/lfw/camera.cpp",
      from: `    if (round(position.y) == round(destination.y)) break;`,
      to: `    if (round(position.y) != round(destination.y)) break;`,
    },
    {
      note: "Camera: 方向取反（y）",
      file: "native/lfw/camera.cpp",
      from: `    const double direction_y = position.y > destination.y ? -1 : 1;`,
      to: `    const double direction_y = position.y > destination.y ? 1 : -1;`,
    },
    {
      note: "Camera: 方向重置判断取反（y）",
      file: "native/lfw/camera.cpp",
      from: `    if (sign(velocity.y) != direction_y) velocity.y = 0;`,
      to: `    if (sign(velocity.y) == direction_y) velocity.y = 0;`,
    },
    {
      note: "Camera: 加速分支用减号（y）",
      file: "native/lfw/camera.cpp",
      from: `      velocity.y += acc_y * direction_y;`,
      to: `      velocity.y -= acc_y * direction_y;`,
    },
    {
      note: "Camera: 封顶分支写成加速量（y）",
      file: "native/lfw/camera.cpp",
      from: `      velocity.y = max_vy;`,
      to: `      velocity.y = acc_y * direction_y;`,
    },
    {
      note: "Camera: 收敛用的 max / min 调换（y）",
      file: "native/lfw/camera.cpp",
      from: `    if (direction_y < 0)
      position.y = max(destination.y, position.y + velocity.y);
    else
      position.y = min(destination.y, position.y + velocity.y);`,
      to: `    if (direction_y < 0)
      position.y = min(destination.y, position.y + velocity.y);
    else
      position.y = max(destination.y, position.y + velocity.y);`,
    },
    {
      note: "Camera: 位移用减号（y）",
      file: "native/lfw/camera.cpp",
      from: `      position.y = min(destination.y, position.y + velocity.y);`,
      to: `      position.y = min(destination.y, position.y - velocity.y);`,
    },
  ],
};
