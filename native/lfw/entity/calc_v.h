#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace entity {

double calc_v(double current, double value, const Value& mode, const Value& acc,
              const Value& direction);

}
}
