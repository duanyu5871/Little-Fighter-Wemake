#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Walking : public CharacterState_Base {
 public:
  explicit CharacterState_Walking(Value state = Value(static_cast<double>(StateEnum::Walking)))
      : CharacterState_Base(std::move(state)) {}
  void update(IStateEntity& e) override;
};

}
}
