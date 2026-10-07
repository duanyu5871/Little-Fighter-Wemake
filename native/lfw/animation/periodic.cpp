#include "lfw/animation/periodic.h"

#include <cmath>

namespace lfw {

namespace {

// TS `is_num`。
bool is_num(double v) { return !std::isnan(v) && std::isfinite(v); }

// `Number.MAX_SAFE_INTEGER`：周期动画的时长。
constexpr double kMaxSafeInteger = 9007199254740991.0;

}  // namespace

Periodic::Periodic(double bottom, double height, double scale) {
  set_duration(kMaxSafeInteger);
  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  if (is_num(scale)) _s = scale;
}

Periodic& Periodic::set_offset(double v) {
  offset = v;
  return *this;
}

Periodic& Periodic::set_scale(double v) {
  if (is_num(v)) _s = v;
  return *this;
}

Periodic& Periodic::set(double bottom, double height, double scale) {
  if (is_num(bottom)) _b = bottom;
  if (is_num(height)) _h = height;
  if (is_num(scale)) _s = scale;
  calc();
  return *this;
}

void Periodic::set_bottom(double v) {
  if (is_num(v)) _b = v;
}

void Periodic::set_height(double v) {
  if (is_num(v)) _h = v;
}

Animation& Periodic::calc() {
  set_value(method(offset + time() * scale()));
  return *this;
}

}
