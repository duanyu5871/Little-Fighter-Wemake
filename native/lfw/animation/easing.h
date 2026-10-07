#pragma once

#include <functional>

#include "lfw/animation/animation.h"

namespace lfw {

// TS `IEasing`（`src/LFW/utils/easing/IEasing.ts`）：`(factor, val_1, val_2) => number`。
using IEasing = std::function<double(double factor, double val_1, double val_2)>;

// Mirrors `src/LFW/animation/Easing.ts`：缺省缓动是 `ease_in_out_sine`；
// TS 的属性 setter `set easing(v)`（无返回）与同名方法 `set_easing(v)`（返回 this）
// 在端口并成一个 `set_easing`（都只是赋值，链式返回更好用）。
// `set(begin, end)` 的 `is_num` 守卫照抄（非数保留旧值；C++ 侧只可能 NaN / ±Infinity）。
class Easing : public Animation {
 public:
  explicit Easing(double begin = 0, double end = 1);

  Easing& set(double begin, double end);
  Easing& set_easing(IEasing v);

  double val_1() const { return _val_1; }
  double val_2() const { return _val_2; }
  void set_val_1(double v) { _val_1 = v; }
  void set_val_2(double v) { _val_2 = v; }
  const IEasing& easing() const { return _easing; }

  Animation& calc() override;

 private:
  double _val_1 = 0;
  double _val_2 = 1;
  IEasing _easing;
};

}
