#pragma once

#include "lfw/animation/animation.h"

namespace lfw {

// Mirrors `src/LFW/animation/Periodic.ts`：周期动画基类（时长 = `Number.MAX_SAFE_INTEGER`）。
//   * `offset` 在 TS 里是**公开字段**（不是访问器）⇒ 端口同名公开成员；`set_offset` 方法保留。
//   * `bottom` / `height` / `scale` 的属性 setter 都带 `is_num` 守卫（非数保留旧值）；
//     TS 的 `set_scale(v)`（方法）与 `set scale(v)`（属性）合并成一个带守卫的 `set_scale`。
//   * 构造函数与 `set(bottom, height, scale)` 三项都守；`set` 末尾还会 `calc()`。
//   * 子类只实现 `method(v)`；`calc` = `method(offset + time * scale)`。
class Periodic : public Animation {
 public:
  explicit Periodic(double bottom = 0, double height = 1, double scale = 1);

  double offset = 0;

  Periodic& set_offset(double v);
  Periodic& set_scale(double v);
  Periodic& set(double bottom, double height, double scale);

  double bottom() const { return _b; }
  void set_bottom(double v);
  double height() const { return _h; }
  void set_height(double v);
  double scale() const { return _s; }

  virtual double method(double v) = 0;
  Animation& calc() override;

 private:
  double _b = 0;
  double _h = 1;
  double _s = 1;
};

}
