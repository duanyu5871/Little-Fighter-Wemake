#include "lfw/animation/easing.h"

#include <cmath>
#include <utility>

#include "lfw/utils/easing/ease_in_out_sine.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

namespace {

// TS `is_num`：`typeof v === "number" && !is_nan(v) && is_finite(v)`。
bool is_num(double v) { return !std::isnan(v) && std::isfinite(v); }

}  // namespace

Easing::Easing(double begin, double end) {
  _easing = ease_in_out_sine;
  _val_1 = begin;
  _val_2 = end;
}

Easing& Easing::set(double begin, double end) {
  _val_1 = is_num(begin) ? begin : _val_1;
  _val_2 = is_num(end) ? end : _val_2;
  return *this;
}

Easing& Easing::set_easing(IEasing v) {
  _easing = std::move(v);
  return *this;
}

Animation& Easing::calc() {
  const bool is_end = done();
  const double time = this->time();
  const double duration = this->duration();
  const double val_1 = this->_val_1;
  const double val_2 = this->_val_2;
  const bool reverse = this->reverse();
  if (val_1 == val_2) {
    set_value(val_1);
    return *this;
  }
  if (is_end) {
    set_value(reverse ? val_1 : val_2);
    return *this;
  }
  const double factor = clamp(time / duration, 0, 1);
  set_value(_easing(factor, val_1, val_2));
  return *this;
}

}
