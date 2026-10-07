#include "lfw/animation/loop.h"

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

namespace {

// `Number.MAX_SAFE_INTEGER`
constexpr double kMaxSafeInteger = 9007199254740991.0;

}  // namespace

Loop& Loop::set_count(double v) {
  _count = clamp(floor(v), 0, _times);
  return *this;
}

Loop& Loop::set_times(double v) {
  _times = clamp(floor(v), 0, kMaxSafeInteger);
  return *this;
}

// ⚠ 先 `times` 后 `count`（TS 里 count 的 clamp 用**新** times）。
Loop& Loop::set(double count, double times) {
  set_times(times);
  set_count(count);
  return *this;
}

Loop& Loop::reset() { return set_count(0); }

bool Loop::continue_() {
  if (_times <= 0) return true;
  if (_count >= _times) return false;
  ++_count;
  return true;
}

bool Loop::done() const { return _times > 0 && _count >= _times; }

}
