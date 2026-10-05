#pragma once

#include "lfw/core/value.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

// TS: `class FPS`（`src/LFW/base/FPS.ts`）：帧率的指数滑动平均。
class FPS {
 public:
  explicit FPS(double retention = 0.99) { _retention = clamp(retention, 0, 0.99); }

  double value() const { return _value; }
  // TS 里是私有字段；harness 要读回（同 `Ticker` 的 `_base` 那类私有探针）。
  double duration() const { return _duration; }
  double retention() const { return _retention; }

  void update(double dt) {
    if (truthy(Value(_duration))) {
      _duration = _duration * _retention + dt * (1 - _retention);
    } else {
      _duration = dt;
    }
    _value = 1000 / _duration;
  }

  void reset() {
    _value = 0;
    _duration = 0;
  }

 private:
  double _value = 0;
  double _duration = 0;
  double _retention = 0.99;
};

}
