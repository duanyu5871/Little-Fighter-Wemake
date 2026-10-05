#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/frame_id.h"

namespace lfw {

// `defines/EMPTY_FRAME_INFO`：`BallController.frame` 的初值（TS 里是个冻结字面量）。
// 端口按同样的字段现搭一个；`chase` / `behavior` 之类不给 ⇒ 读出来是 `undefined`（同 TS）。
inline const Value& empty_frame_info() {
  static const Value v = [] {
    Object pic;
    pic.set(u"tex", Value(std::u16string()));
    pic.set(u"x", Value(0.0));
    pic.set(u"y", Value(0.0));
    pic.set(u"w", Value(0.0));
    pic.set(u"h", Value(0.0));

    Object o;
    o.set(u"id", Value(std::u16string(frame_id::kNone)));
    o.set(u"name", Value(std::u16string(u"EMPTY_FRAME_INFO")));
    o.set(u"pic", Value(std::make_shared<Object>(pic)));
    o.set(u"width", Value(0.0));
    o.set(u"height", Value(0.0));
    o.set(u"state", Value(std::nan("")));
    o.set(u"wait", Value(0.0));
    o.set(u"centerx", Value(0.0));
    o.set(u"centery", Value(0.0));
    const Value* nf = defines::find(u"Defines.NEXT_FRAME_AUTO");
    if (nf != nullptr) o.set(u"next", *nf);
    return Value(std::make_shared<Object>(o));
  }();
  return v;
}

}
