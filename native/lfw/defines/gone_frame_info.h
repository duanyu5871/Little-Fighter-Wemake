#pragma once

#include <cmath>
#include <memory>

#include "lfw/core/value.h"
#include "lfw/defines/frame_id.h"

namespace lfw {

inline const Value& gone_frame_info() {
  static const Value v = [] {
    Object pic;
    pic.set(u"tex", Value(std::u16string(u"")));
    pic.set(u"x", Value(0.0));
    pic.set(u"y", Value(0.0));
    pic.set(u"w", Value(0.0));
    pic.set(u"h", Value(0.0));
    Object next;
    next.set(u"id", Value(std::u16string(frame_id::kGone)));
    Object o;
    o.set(u"id", Value(std::u16string(frame_id::kGone)));
    o.set(u"name", Value(std::u16string(u"GONE_FRAME_INFO")));
    o.set(u"pic", Value(std::make_shared<Object>(pic)));
    o.set(u"width", Value(0.0));
    o.set(u"height", Value(0.0));
    o.set(u"state", Value(std::nan("")));
    o.set(u"wait", Value(0.0));
    o.set(u"next", Value(std::make_shared<Object>(next)));
    o.set(u"centerx", Value(0.0));
    o.set(u"centery", Value(0.0));
    o.set(u"no_shadow", Value(1.0));
    return Value(std::make_shared<Object>(o));
  }();
  return v;
}

}
