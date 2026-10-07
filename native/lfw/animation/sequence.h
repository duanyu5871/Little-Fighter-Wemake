#pragma once

#include <optional>
#include <vector>

#include "lfw/animation/animation.h"

namespace lfw {

// Mirrors `src/LFW/animation/Sequence.ts`：把一串动画首尾相接。
//   * TS 的 `anims` 是**引用数组**（不持有）⇒ 端口给裸指针 `std::vector<Animation*>`，
//     生命周期由调用方管（台面里挂全局表）。
//   * 构造即 `duration = Σ 子动画时长` 然后 `start()`。
//   * ⚠ `start` / `end` 里取「头还是尾」用的是**形参** `reverse` 原值（`nullopt` 算假）——
//     即使当前 `reverse()` 为真，`start()` 还是取头。TS 就这怪癖，照抄。
//   * `calc()` 的两个边界（`time >= duration` / `time <= 0`）直接掐头尾子动画；
//     逆放分支里 `duration` 是**局部副本**逐个减（TS 原文）。
//   * `readonly anims` 只是引用不可换、数组本身可改（UI 层会 push）⇒ 给可写视图。
class Sequence : public Animation {
 public:
  explicit Sequence(std::vector<Animation*> anims = {});

  Animation& start(std::optional<bool> reverse = std::nullopt) override;
  Animation& end(std::optional<bool> reverse = std::nullopt) override;
  Animation& calc() override;

  Animation* curr_anim() const { return _curr_anim; }
  const std::vector<Animation*>& anims() const { return _anims; }
  std::vector<Animation*>& anims() { return _anims; }

 private:
  std::vector<Animation*> _anims;
  Animation* _curr_anim = nullptr;
};

}
