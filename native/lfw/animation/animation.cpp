#include "lfw/animation/animation.h"

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

Animation& Animation::set_fill_mode(double v) {
  // TS: `if (v != 1 && v != 0) debugger;` —— `debugger` 无操作。
  _fill_mode = (v == 1 || v == 0) ? v : _fill_mode;
  return *this;
}

Animation& Animation::set_count(double v) {
  _loop.set_count(v);
  return *this;
}

Animation& Animation::set_times(double v) {
  _loop.set_times(v);
  return *this;
}

Animation& Animation::set_direction(double v) {
  // TS: `if (v !== -1 && v !== 1) debugger;` —— `debugger` 无操作。
  _direction = (v == -1 || v == 1) ? v : _direction;
  return *this;
}

Animation& Animation::set_reverse(bool v) {
  set_direction(v ? -1 : 1);
  return *this;
}

Animation& Animation::set_duration(double v) {
  _duration = max(0.0, v);
  _time = clamp(_time, 0, _duration);
  return *this;
}

Animation& Animation::set_value(double v) {
  _value = v;
  return *this;
}

Animation& Animation::set_time(double v) {
  _time = clamp(v, 0, duration());
  return *this;
}

bool Animation::done() const {
  const double duration = this->duration();
  if (duration <= 0) return true;
  return _loop.done();
}

Animation& Animation::start(std::optional<bool> reverse) {
  const bool rev = reverse.value_or(this->reverse());
  _loop.reset();
  set_reverse(rev);
  set_time(rev ? duration() : 0);
  return *this;
}

Animation& Animation::end(std::optional<bool> reverse) {
  const bool rev = reverse.value_or(this->reverse());
  set_count(max(0.0, times()));
  set_reverse(rev);
  set_time(rev ? 0 : duration());
  return *this;
}

Animation& Animation::calc() {
  set_value(duration() == 0 ? value() : time() / duration());
  return *this;
}

Animation& Animation::update(double dt) {
  if (done()) return *this;
  const double duration = this->duration();
  const double direction = this->direction();
  double time = this->time() + direction * dt;
  if (time <= 0) {
    do {
      _loop.continue_();
      time += duration;
    } while (time <= 0);
    if (done() && _fill_mode) time = 0;
  } else if (time >= duration) {
    do {
      _loop.continue_();
      time -= duration;
    } while (time >= duration);
    if (done() && _fill_mode) time = duration;
  }
  set_time(clamp(time, 0, duration));
  calc();
  return *this;
}

Animation& Animation::auto_trip(bool reverse, double dt) {
  if (this->reverse() == reverse) return update(dt);
  if (done()) {
    start(reverse);
  } else {
    set_reverse(reverse);
  }
  return update(dt);
}

}
