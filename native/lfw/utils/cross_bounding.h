#pragma once

#include "lfw/defines/i_bounding.h"
#include "lfw/utils/math/base.h"

namespace lfw {

inline Bounding cross_bounding(const Bounding& r0, const Bounding& r1) {
  Bounding ret;
  ret.left = max(r0.left, r1.left);
  ret.right = min(r0.right, r1.right);
  ret.bottom = max(r0.bottom, r1.bottom);
  ret.top = min(r0.top, r1.top);
  ret.far = max(r0.far, r1.far);
  ret.near = min(r0.near, r1.near);
  return ret;
}

}
