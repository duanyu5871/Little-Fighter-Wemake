#pragma once

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {

inline bool float_equal(double x, double y) { return round_float(abs(x - y)) == 0; }
inline bool equal(double x, double y) { return round_float(x - y) == 0; }
inline bool eqgt(double x, double y) { return round_float(x - y) >= 0; }
inline bool eqlt(double x, double y) { return round_float(x - y) <= 0; }

}
