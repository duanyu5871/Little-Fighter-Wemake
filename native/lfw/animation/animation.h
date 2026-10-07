#pragma once

#include <optional>

#include "lfw/animation/loop.h"

namespace lfw {

// Mirrors `src/LFW/animation/Animation.ts`：值/时间/时长/方向/填充模式 + 内嵌 `Loop`。
// 形状：
//   * TS 的属性对（`get count` / `set count`）⇒ 端口 getter 方法 + `set_*` 方法；
//     `set_*` 保留 TS 的**链式**返回（`this` 在 TS 链路里用到过）。
//   * `start(reverse = this.reverse)` / `end(reverse = this.reverse)` 的缺省值来自
//     **调用那一刻**的 `reverse` ⇒ 端口给 `std::optional<bool>`（`nullopt` = 取值时的
//     `reverse()`）。
//   * TS 里两处 `debugger;`（`set_fill_mode` / `set_direction` 的类型守卫）只是开发
//     断点，端口忽略；守卫分支本身照抄（非法值保留旧值）。
//   * `update` / `auto_trip` 不是虚方法（TS 没有子类覆盖）；`start` / `end` / `calc`
//     是（Delay / Easing / Periodic / Sequence 覆盖）。
class Animation {
 public:
  Animation() = default;
  virtual ~Animation() = default;

  double fill_mode() const { return _fill_mode; }
  Animation& set_fill_mode(double v);
  double count() const { return _loop.count(); }
  Animation& set_count(double v);
  double times() const { return _loop.times(); }
  Animation& set_times(double v);
  double direction() const { return _direction; }
  Animation& set_direction(double v);
  bool reverse() const { return _direction == -1; }
  Animation& set_reverse(bool v);
  double duration() const { return _duration; }
  Animation& set_duration(double v);
  double value() const { return _value; }
  Animation& set_value(double v);
  double time() const { return _time; }
  Animation& set_time(double v);
  bool done() const;
  Loop& loop() { return _loop; }
  const Loop& loop() const { return _loop; }

  virtual Animation& start(std::optional<bool> reverse = std::nullopt);
  virtual Animation& end(std::optional<bool> reverse = std::nullopt);
  virtual Animation& calc();
  Animation& update(double dt);
  Animation& auto_trip(bool reverse, double dt);

 private:
  double _value = 0;
  double _time = 0;
  double _duration = 0;
  double _direction = 1;  // TS `-1 | 1`
  double _fill_mode = 1;  // TS `1 | 0`
  Loop _loop;
};

}
