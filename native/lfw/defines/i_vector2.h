#pragma once

namespace lfw {

// Mirrors `src/LFW/defines/IVector2.ts`。TS 的向量实例来自宿主（`Ditto.vec2`），所以端口只实现
// 用到的部分：`Camera` 只读写 `x` / `y` 两项。`add` / `sub` / `length` / `clone` /
// `normalize` / `equals` 随用到的物理刀再补。
struct Vector2 {
  double x = 0;
  double y = 0;

  Vector2() = default;
  Vector2(double px, double py) : x(px), y(py) {}

  void set(double px, double py) {
    x = px;
    y = py;
  }
};

}
