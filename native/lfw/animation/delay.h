#pragma once

#include "lfw/animation/animation.h"

namespace lfw {

// Mirrors `src/LFW/animation/Delay.ts`：占位动画 —— 构造后值固定，`calc` 不改值。
class Delay : public Animation {
 public:
  explicit Delay(double value);
  Animation& calc() override;
};

}
