#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct ItrVelocity {
  double x;
  double y;
  double z;
  Value x_direction;
};

ItrVelocity calc_itr_velocity(const Value& collision);

}
}
