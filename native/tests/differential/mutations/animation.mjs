// `lfw/animation/*`（动画家族第一支）的变异档。
//
// 用例：`cases/animation/{loop,anim,periodic,easing,sequence}.txt`（任一锁住即可）。
//
// 有意不覆盖（不可观察 / 走不到 / 会挂死）：
//   * `Loop::continue_` 的 `_times <= 0`：返回 `true` / 调用后计数不变，`times == 0` 时
//     `<= 0` 与 `< 0` 两条路的**可观察面**相同（count 都不动）⇒ 改不改一样。
//   * `Easing::calc` 里 `clamp(time / duration, 0, 1)`：`time` 已被 `set_time` 夹在
//     `[0, duration]`，因子必在范围内 ⇒ 夹取不可观察（NaN 时两条路都给 NaN 因子）。
//   * `Animation::update` 的上/下循环里 `time ±= duration` 改成 `±= 0`：`duration == 0`
//     时死循环、`duration > 0` 时……不，改成 0 就会死循环 ⇒ 会挂死，不选。
//   * `Animation::update` 开头的 `if (done()) return *this;`：去掉后在全零时长动画上
//     会陷入死循环（本档只保留安全的等价改写，见 update-calc 一条）。
//   * 三角函数换极（`tan` 在 125/500/1000/137 已避开 π/2 奇点）：改 `/[0-9]+` 的
//     正负号会撞极 ⇒ 只做安全改写。
//   * `Animation` / `Easing` / `Periodic` 的**头文件缺省实参**：台面 `mk` 一律显式
//     传参（`easing [begin] [end]` 等缺失时自己补 0/1）⇒ 头里改缺省值对两边都不可见。
//   * `easing.cpp` 里换 `ease_linearity` 这类变异会**编译不过**（缺 include）⇒ 换成
//     等价的 lambda（本来就是 `std::function`）。
export default {
  subject: "animation",
  cases: ["loop", "anim", "periodic", "easing", "sequence"],
  mutations: [
    // ---------------------------------------------------------------- loop.cpp
    {
      note: "Loop::set_count(): 去掉 floor（1.9 / 0.5 直接进 clamp）",
      file: "native/lfw/animation/loop.cpp",
      from: `Loop& Loop::set_count(double v) {
  _count = clamp(floor(v), 0, _times);`,
      to: `Loop& Loop::set_count(double v) {
  _count = clamp(v, 0, _times);`,
    },
    {
      note: "Loop::set_count(): 上界换成 MAX_SAFE（99 不再被 times 夹）",
      file: "native/lfw/animation/loop.cpp",
      from: `  _count = clamp(floor(v), 0, _times);`,
      to: `  _count = clamp(floor(v), 0, kMaxSafeInteger);`,
    },
    {
      note: "Loop::set_count(): 下界放开（-3 原样保留）",
      file: "native/lfw/animation/loop.cpp",
      from: `  _count = clamp(floor(v), 0, _times);`,
      to: `  _count = clamp(floor(v), -kMaxSafeInteger, _times);`,
    },
    {
      note: "Loop::set_times(): 去掉 floor（3.7 不再取整）",
      file: "native/lfw/animation/loop.cpp",
      from: `  _times = clamp(floor(v), 0, kMaxSafeInteger);`,
      to: `  _times = clamp(v, 0, kMaxSafeInteger);`,
    },
    {
      note: "Loop::set_times(): 上界换成 1e300（Infinity 不再收成 MAX_SAFE）",
      file: "native/lfw/animation/loop.cpp",
      from: `  _times = clamp(floor(v), 0, kMaxSafeInteger);`,
      to: `  _times = clamp(floor(v), 0, 1e300);`,
    },
    {
      note: "Loop::set_times(): 下界放开（-2 原样保留）",
      file: "native/lfw/animation/loop.cpp",
      from: `  _times = clamp(floor(v), 0, kMaxSafeInteger);`,
      to: `  _times = clamp(floor(v), -kMaxSafeInteger, kMaxSafeInteger);`,
    },
    {
      note: "Loop::set(): 次序对调（count 用旧 times 夹）",
      file: "native/lfw/animation/loop.cpp",
      from: `Loop& Loop::set(double count, double times) {
  set_times(times);
  set_count(count);`,
      to: `Loop& Loop::set(double count, double times) {
  set_count(count);
  set_times(times);`,
    },
    {
      note: "Loop::continue_(): 满额判定 >= 改成 >（count == times 还能再推）",
      file: "native/lfw/animation/loop.cpp",
      from: `  if (_count >= _times) return false;`,
      to: `  if (_count > _times) return false;`,
    },
    {
      note: "Loop::done(): times > 0 改成 >= 0（times 为 0 也算完）",
      file: "native/lfw/animation/loop.cpp",
      from: `bool Loop::done() const { return _times > 0 && _count >= _times; }`,
      to: `bool Loop::done() const { return _times >= 0 && _count >= _times; }`,
    },
    {
      note: "Loop::done(): count >= times 改成 >（刚好满额不算完）",
      file: "native/lfw/animation/loop.cpp",
      from: `bool Loop::done() const { return _times > 0 && _count >= _times; }`,
      to: `bool Loop::done() const { return _times > 0 && _count > _times; }`,
    },
    {
      note: "Loop::reset(): 不再清零（原样返回）",
      file: "native/lfw/animation/loop.cpp",
      from: `Loop& Loop::reset() { return set_count(0); }`,
      to: `Loop& Loop::reset() { return *this; }`,
    },
    {
      note: "Loop 缺省 times 从 1 变 2",
      file: "native/lfw/animation/loop.h",
      from: `  double _times = 1;`,
      to: `  double _times = 2;`,
    },

    // ----------------------------------------------------------- animation.cpp
    {
      note: "Animation::set_fill_mode(): 去掉守卫（非法值也写进去）",
      file: "native/lfw/animation/animation.cpp",
      from: `  _fill_mode = (v == 1 || v == 0) ? v : _fill_mode;`,
      to: `  _fill_mode = v;`,
    },
    {
      note: "Animation::set_direction(): 去掉守卫",
      file: "native/lfw/animation/animation.cpp",
      from: `  _direction = (v == -1 || v == 1) ? v : _direction;`,
      to: `  _direction = v;`,
    },
    {
      note: "Animation::set_duration(): 去掉 max(0, v)（负数原样保留）",
      file: "native/lfw/animation/animation.cpp",
      from: `  _duration = max(0.0, v);
  _time = clamp(_time, 0, _duration);`,
      to: `  _duration = v;
  _time = clamp(_time, 0, _duration);`,
    },
    {
      note: "Animation::set_duration(): 不再回夹 time（缩短时长后时间不跟着收）",
      file: "native/lfw/animation/animation.cpp",
      from: `  _duration = max(0.0, v);
  _time = clamp(_time, 0, _duration);
  return *this;`,
      to: `  _duration = max(0.0, v);
  return *this;`,
    },
    {
      note: "Animation::set_time(): 去掉夹取",
      file: "native/lfw/animation/animation.cpp",
      from: `  _time = clamp(v, 0, duration());`,
      to: `  _time = v;`,
    },
    {
      note: "Animation::start(): 不再 reset 计数",
      file: "native/lfw/animation/animation.cpp",
      from: `  const bool rev = reverse.value_or(this->reverse());
  _loop.reset();
  set_reverse(rev);`,
      to: `  const bool rev = reverse.value_or(this->reverse());
  set_reverse(rev);`,
    },
    {
      note: "Animation::start(): 起始时间恒为 0（倒放不从末尾起）",
      file: "native/lfw/animation/animation.cpp",
      from: `  set_time(rev ? duration() : 0);`,
      to: `  set_time(0);`,
    },
    {
      note: "Animation::end(): count 少补一位（times - 1）",
      file: "native/lfw/animation/animation.cpp",
      from: `  set_count(max(0.0, times()));`,
      to: `  set_count(times() - 1);`,
    },
    {
      note: "Animation::end(): 落点恒为 0（正放不到末尾）",
      file: "native/lfw/animation/animation.cpp",
      from: `  set_time(rev ? 0 : duration());`,
      to: `  set_time(0);`,
    },
    {
      note: "Animation::calc(): 时长为 0 时不再保持旧值（走 0/0）",
      file: "native/lfw/animation/animation.cpp",
      from: `  set_value(duration() == 0 ? value() : time() / duration());`,
      to: `  set_value(time() / duration());`,
    },
    {
      note: "Animation::update(): 末尾漏掉 calc()",
      file: "native/lfw/animation/animation.cpp",
      from: `  set_time(clamp(time, 0, duration));
  calc();`,
      to: `  set_time(clamp(time, 0, duration));`,
    },
    {
      note: "Animation::update(): 上溢循环不推进会次数",
      file: "native/lfw/animation/animation.cpp",
      from: `    do {
      _loop.continue_();
      time -= duration;`,
      to: `    do {
      time -= duration;`,
    },
    {
      note: "Animation::update(): 下溢循环不推进会次数",
      file: "native/lfw/animation/animation.cpp",
      from: `    do {
      _loop.continue_();
      time += duration;`,
      to: `    do {
      time += duration;`,
    },
    {
      note: "Animation::update(): 上溢 + 填充落点改成 0",
      file: "native/lfw/animation/animation.cpp",
      from: `    if (done() && _fill_mode) time = duration;`,
      to: `    if (done() && _fill_mode) time = 0;`,
    },
    {
      note: "Animation::update(): 下溢 + 填充落点改成 duration",
      file: "native/lfw/animation/animation.cpp",
      from: `    if (done() && _fill_mode) time = 0;`,
      to: `    if (done() && _fill_mode) time = duration;`,
    },
    {
      note: "Animation::auto_trip(): 相等判定反了（该直接推进的走了折返）",
      file: "native/lfw/animation/animation.cpp",
      from: `  if (this->reverse() == reverse) return update(dt);`,
      to: `  if (this->reverse() != reverse) return update(dt);`,
    },
    {
      note: "Animation::auto_trip(): 完播折返不再 start（只翻方向）",
      file: "native/lfw/animation/animation.cpp",
      from: `  if (done()) {
    start(reverse);
  } else {`,
      to: `  if (done()) {
    set_reverse(reverse);
  } else {`,
    },
    {
      note: "Animation 缺省 fill_mode 从 1 变 0",
      file: "native/lfw/animation/animation.h",
      from: `  double _fill_mode = 1;`,
      to: `  double _fill_mode = 0;`,
    },
    {
      note: "Animation 缺省 direction 从 1 变 -1",
      file: "native/lfw/animation/animation.h",
      from: `  double _direction = 1;`,
      to: `  double _direction = -1;`,
    },

    // ------------------------------------------------------------- easing
    {
      note: "Easing 缺省缓动换成常量 lambda（easing.cpp 没 include linearity）",
      file: "native/lfw/animation/easing.cpp",
      from: `  _easing = ease_in_out_sine;`,
      to: `  _easing = [](double, double a, double) { return a; };`,
    },
    {
      note: "Easing::set_easing(): 不再落值（(void)v）",
      file: "native/lfw/animation/easing.cpp",
      from: `Easing& Easing::set_easing(IEasing v) {
  _easing = std::move(v);`,
      to: `Easing& Easing::set_easing(IEasing v) {
  (void)v;`,
    },
    {
      note: "Easing::calc(): 两端相等时值 +1",
      file: "native/lfw/animation/easing.cpp",
      from: `    set_value(val_1);
    return *this;`,
      to: `    set_value(val_1 + 1);
    return *this;`,
    },
    {
      note: "Easing::calc(): 完播时不看 reverse（恒给 val_2）",
      file: "native/lfw/animation/easing.cpp",
      from: `    set_value(reverse ? val_1 : val_2);`,
      to: `    set_value(val_2);`,
    },
    {
      note: "Easing::set(): begin 守卫去掉",
      file: "native/lfw/animation/easing.cpp",
      from: `  _val_1 = is_num(begin) ? begin : _val_1;`,
      to: `  _val_1 = begin;`,
    },
    {
      note: "Easing::set(): end 守卫去掉",
      file: "native/lfw/animation/easing.cpp",
      from: `  _val_2 = is_num(end) ? end : _val_2;`,
      to: `  _val_2 = end;`,
    },

    // ----------------------------------------------------------- periodic
    {
      note: "Periodic 构造：时长不再取 MAX_SAFE（改 0）",
      file: "native/lfw/animation/periodic.cpp",
      from: `  set_duration(kMaxSafeInteger);`,
      to: `  set_duration(0);`,
    },
    {
      note: "Periodic 构造：bottom 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  set_duration(kMaxSafeInteger);
  if (is_num(bottom)) _b = bottom;`,
      to: `  set_duration(kMaxSafeInteger);
  _b = bottom;`,
    },
    {
      note: "Periodic 构造：height 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  if (is_num(scale)) _s = scale;
}

Periodic& Periodic::set_offset(double v) {`,
      to: `  if (is_num(bottom)) _b = bottom;
  _h = height;
  if (is_num(scale)) _s = scale;
}

Periodic& Periodic::set_offset(double v) {`,
    },
    {
      note: "Periodic 构造：scale 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(scale)) _s = scale;
}

Periodic& Periodic::set_offset(double v) {`,
      to: `  _s = scale;
}

Periodic& Periodic::set_offset(double v) {`,
    },
    {
      note: "Periodic::set(): bottom 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `Periodic& Periodic::set(double bottom, double height, double scale) {
  if (is_num(bottom)) _b = bottom;`,
      to: `Periodic& Periodic::set(double bottom, double height, double scale) {
  _b = bottom;`,
    },
    {
      note: "Periodic::set(): height 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  if (is_num(scale)) _s = scale;
  calc();`,
      to: `  if (is_num(bottom)) _b = bottom;
  _h = height;
  if (is_num(scale)) _s = scale;
  calc();`,
    },
    {
      note: "Periodic::set(): scale 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  if (is_num(scale)) _s = scale;
  calc();`,
      to: `  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  _s = scale;
  calc();`,
    },
    {
      note: "Periodic::set(): 末尾不再 calc()",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(scale)) _s = scale;
  calc();
  return *this;`,
      to: `  if (is_num(scale)) _s = scale;
  return *this;`,
    },
    {
      note: "Periodic::set_scale(): 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(v)) _s = v;`,
      to: `  _s = v;`,
    },
    {
      note: "Periodic::set_bottom(): 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(v)) _b = v;`,
      to: `  _b = v;`,
    },
    {
      note: "Periodic::set_height(): 守卫去掉",
      file: "native/lfw/animation/periodic.cpp",
      from: `  if (is_num(v)) _h = v;`,
      to: `  _h = v;`,
    },
    {
      note: "Periodic::calc(): 漏掉 offset",
      file: "native/lfw/animation/periodic.cpp",
      from: `  set_value(method(offset + time() * scale()));`,
      to: `  set_value(method(time() * scale()));`,
    },
    {
      note: "Periodic::calc(): 漏掉 scale",
      file: "native/lfw/animation/periodic.cpp",
      from: `  set_value(method(offset + time() * scale()));`,
      to: `  set_value(method(offset + time()));`,
    },

    // ------------------------------------------------------------ cosine
    {
      note: "Cosine::method(): 漏掉 offset",
      file: "native/lfw/animation/cosine.cpp",
      from: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (cos(v * 2 * PI / 1000) + 1) / 2) + bottom();`,
    },
    {
      note: "Cosine::method(): 漏掉 height",
      file: "native/lfw/animation/cosine.cpp",
      from: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return ((cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
    },
    {
      note: "Cosine::method(): +1 变 +2",
      file: "native/lfw/animation/cosine.cpp",
      from: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 2) / 2) + bottom();`,
    },
    {
      note: "Cosine::method(): /2 变 /4",
      file: "native/lfw/animation/cosine.cpp",
      from: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 4) + bottom();`,
    },
    {
      note: "Cosine::method(): + bottom() 变 - bottom()",
      file: "native/lfw/animation/cosine.cpp",
      from: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (cos(offset + v * 2 * PI / 1000) + 1) / 2) - bottom();`,
    },

    // ------------------------------------------------------------- sine
    {
      note: "Sine::method(): 周期倍率 /1000 变 /2000",
      file: "native/lfw/animation/sine.cpp",
      from: `  return (height() * (sin(v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (sin(v * 2 * PI / 2000) + 1) / 2) + bottom();`,
    },
    {
      note: "Sine::method(): 把 offset 也加了进去（TS 是漏的）",
      file: "native/lfw/animation/sine.cpp",
      from: `  return (height() * (sin(v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (sin(offset + v * 2 * PI / 1000) + 1) / 2) + bottom();`,
    },
    {
      note: "Sine::method(): 漏掉 bottom",
      file: "native/lfw/animation/sine.cpp",
      from: `  return (height() * (sin(v * 2 * PI / 1000) + 1) / 2) + bottom();`,
      to: `  return (height() * (sin(v * 2 * PI / 1000) + 1) / 2);`,
    },

    // ---------------------------------------------------------- tangent
    {
      note: "Tangent::method(): 周期倍率 /1000 变 /2000",
      file: "native/lfw/animation/tangent.cpp",
      from: `double Tangent::method(double v) { return tan(v * 2 * PI / 1000); }`,
      to: `double Tangent::method(double v) { return tan(v * 2 * PI / 2000); }`,
    },
    {
      note: "Tangent::method(): 把 bottom/height 也算上（TS 全忽略）",
      file: "native/lfw/animation/tangent.cpp",
      from: `double Tangent::method(double v) { return tan(v * 2 * PI / 1000); }`,
      to: `double Tangent::method(double v) { return bottom() + height() * tan(v * 2 * PI / 1000); }`,
    },

    // ---------------------------------------------------------- sequence
    {
      note: "Sequence 构造：总时长减半",
      file: "native/lfw/animation/sequence.cpp",
      from: `  for (Animation* a : _anims) sum += a->duration();`,
      to: `  for (Animation* a : _anims) sum += a->duration() * 0.5;`,
    },
    {
      note: "Sequence::start(): 恒取队首（倒放不从队尾起）",
      file: "native/lfw/animation/sequence.cpp",
      from: `    _curr_anim = _anims[rev ? _anims.size() - 1 : 0];`,
      to: `    _curr_anim = _anims[0];`,
    },
    {
      note: "Sequence::end(): 恒取队首",
      file: "native/lfw/animation/sequence.cpp",
      from: `    _curr_anim = _anims[rev ? 0 : _anims.size() - 1];`,
      to: `    _curr_anim = _anims[0];`,
    },
    {
      note: "Sequence::start(): rev 改为沿用**当前** reverse（TS 是看形参）",
      file: "native/lfw/animation/sequence.cpp",
      from: `  Animation::start(reverse);
  const bool rev = reverse.value_or(false);`,
      to: `  Animation::start(reverse);
  const bool rev = reverse.value_or(this->reverse());`,
    },
    {
      note: "Sequence::end(): 缺省 rev 恒为 true",
      file: "native/lfw/animation/sequence.cpp",
      from: `  Animation::end(reverse);
  const bool rev = reverse.value_or(false);`,
      to: `  Animation::end(reverse);
  const bool rev = reverse.value_or(true);`,
    },
    {
      note: "Sequence::calc(): 空表守卫改成落值 1",
      file: "native/lfw/animation/sequence.cpp",
      from: `  if (_anims.empty()) return *this;`,
      to: `  if (_anims.empty()) {
    set_value(1);
    return *this;
  }`,
    },
    {
      note: "Sequence::calc(): 上边界 >= 改 >（正好到末尾不进兜底）",
      file: "native/lfw/animation/sequence.cpp",
      from: `  if (time >= duration) {`,
      to: `  if (time > duration) {`,
    },
    {
      note: "Sequence::calc(): 下边界 <= 改 <",
      file: "native/lfw/animation/sequence.cpp",
      from: `  if (time <= 0) {`,
      to: `  if (time < 0) {`,
    },
    {
      note: "Sequence::calc(): 逆放段判定 > 改 >=",
      file: "native/lfw/animation/sequence.cpp",
      from: `      if (time > duration) {`,
      to: `      if (time >= duration) {`,
    },
    {
      note: "Sequence::calc(): 正放段判定 > 改 >=",
      file: "native/lfw/animation/sequence.cpp",
      from: `      if (anim->duration() > time) {`,
      to: `      if (anim->duration() >= time) {`,
    },
    {
      note: "Sequence::calc(): 逆放段漏掉 set_value",
      file: "native/lfw/animation/sequence.cpp",
      from: `        anim->set_time(time - duration);
        set_value(anim->calc().value());`,
      to: `        anim->set_time(time - duration);
        anim->calc();`,
    },
    {
      note: "Sequence::calc(): 正放段漏掉 set_value",
      file: "native/lfw/animation/sequence.cpp",
      from: `        anim->set_time(time);
        set_value(anim->calc().value());`,
      to: `        anim->set_time(time);
        anim->calc();`,
    },
    {
      note: "Sequence::calc(): 逆放段扣减方向反了（duration +=）",
      file: "native/lfw/animation/sequence.cpp",
      from: `      duration -= anim->duration();`,
      to: `      duration += anim->duration();`,
    },
    {
      note: "Sequence::calc(): 正放段扣减方向反了（time +=）",
      file: "native/lfw/animation/sequence.cpp",
      from: `      time -= anim->duration();`,
      to: `      time += anim->duration();`,
    },

    // ------------------------------------------------------------- delay
    {
      note: "Delay 构造：不再把值写给动画 （(void)value）",
      file: "native/lfw/animation/delay.cpp",
      from: `Delay::Delay(double value) { set_value(value); }`,
      to: `Delay::Delay(double value) { (void)value; }`,
    },
    {
      note: "Delay::calc(): 改成走基类（时间/时长，而非恒值）",
      file: "native/lfw/animation/delay.cpp",
      from: `Animation& Delay::calc() { return *this; }`,
      to: `Animation& Delay::calc() { return Animation::calc(); }`,
    },
  ],
};
