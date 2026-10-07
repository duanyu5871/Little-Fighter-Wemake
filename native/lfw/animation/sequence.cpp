#include "lfw/animation/sequence.h"

#include <cstddef>
#include <utility>

namespace lfw {

Sequence::Sequence(std::vector<Animation*> anims) : _anims(std::move(anims)) {
  double sum = 0;
  for (Animation* a : _anims) sum += a->duration();
  set_duration(sum);
  start();
}

Animation& Sequence::start(std::optional<bool> reverse) {
  Animation::start(reverse);
  const bool rev = reverse.value_or(false);
  if (_anims.empty()) {
    _curr_anim = nullptr;
  } else {
    _curr_anim = _anims[rev ? _anims.size() - 1 : 0];
  }
  return *this;
}

Animation& Sequence::end(std::optional<bool> reverse) {
  Animation::end(reverse);
  const bool rev = reverse.value_or(false);
  if (_anims.empty()) {
    _curr_anim = nullptr;
  } else {
    _curr_anim = _anims[rev ? 0 : _anims.size() - 1];
  }
  return *this;
}

Animation& Sequence::calc() {
  double time = this->time();
  const bool reverse = this->reverse();
  double duration = this->duration();
  if (_anims.empty()) return *this;
  if (time >= duration) {
    Animation* a = _anims.back();
    _curr_anim = a;
    a->set_time(a->duration());
    set_value(a->calc().value());
    return *this;
  }
  if (time <= 0) {
    Animation* a = _anims.front();
    _curr_anim = a;
    a->set_time(0);
    set_value(a->calc().value());
    return *this;
  }
  const std::size_t len = _anims.size();
  if (reverse) {
    for (long idx = static_cast<long>(len) - 1; idx >= 0; --idx) {
      Animation* anim = _anims[static_cast<std::size_t>(idx)];
      duration -= anim->duration();
      if (time > duration) {
        anim->set_time(time - duration);
        set_value(anim->calc().value());
        _curr_anim = anim;
        break;
      }
    }
  } else {
    for (std::size_t idx = 0; idx < len; ++idx) {
      Animation* anim = _anims[idx];
      if (anim->duration() > time) {
        anim->set_time(time);
        set_value(anim->calc().value());
        _curr_anim = anim;
        break;
      }
      time -= anim->duration();
    }
  }
  return *this;
}

}
