#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Caught : public CharacterState_Base {
 public:
  explicit CharacterState_Caught(Value state = Value(static_cast<double>(StateEnum::Caught)));
  void update(IStateEntity& e) override;
};

class CharacterState_Rowing : public CharacterState_Base {
 public:
  explicit CharacterState_Rowing(Value state = Value(static_cast<double>(StateEnum::Rowing)));
};

}
}
