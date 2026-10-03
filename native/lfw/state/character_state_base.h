#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/state/state_base.h"

namespace lfw {
namespace state {

class CharacterState_Base : public State_Base {
 public:
  explicit CharacterState_Base(Value state);

  void update(IStateEntity& e) override;
};

}
}
