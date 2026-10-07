#pragma once

#include "lfw/animation/periodic.h"

namespace lfw {

// Mirrors `src/LFW/animation/Sine.ts`：`(height * (sin(v * 2π/1000) + 1) / 2) + bottom`
// —— ⚠ **不带 `offset`**（TS 原文如此，余弦才用 offset）。
class Sine : public Periodic {
 public:
  using Periodic::Periodic;

  double method(double v) override;
};

}
