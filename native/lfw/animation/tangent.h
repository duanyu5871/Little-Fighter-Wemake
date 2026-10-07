#pragma once

#include "lfw/animation/periodic.h"

namespace lfw {

// Mirrors `src/LFW/animation/Tangent.ts`：`tan(v * 2π/1000)` —— ⚠ `bottom` / `height` /
// `scale`（含 `offset`）**都不参与**（TS 原文如此，照抄）。
class Tangent : public Periodic {
 public:
  using Periodic::Periodic;

  double method(double v) override;
};

}
