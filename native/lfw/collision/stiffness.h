#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct Stiffness {
  Value motionless;
  Value shaking;
};

Stiffness calc_stiffness(const Value& collision);

}
}
