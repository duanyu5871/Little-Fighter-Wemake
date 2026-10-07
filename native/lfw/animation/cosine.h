#pragma once

#include "lfw/animation/periodic.h"

namespace lfw {

// Mirrors `src/LFW/animation/Cosine.ts`：`(height * (cos(offset + v * 2π/1000) + 1) / 2) + bottom`
// （实参算序照抄：`((v * 2) * PI) / 1000`）。无自带构造 ⇒ 继承 `Periodic` 的三个参数。
class Cosine : public Periodic {
 public:
  using Periodic::Periodic;

  double method(double v) override;
};

}
