#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/state/state_base.h"

namespace lfw {
namespace state {

class WeaponState_Base : public State_Base {
 public:
  explicit WeaponState_Base(Value state);
  void update(IStateEntity& e) override;
  void hit_ground_rebouncing(IStateEntity& e, const Value& nf, const Value& velocity);
};

}
}
