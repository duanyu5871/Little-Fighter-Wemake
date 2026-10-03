#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Standing : public CharacterState_Base {
 public:
  explicit CharacterState_Standing(Value state = Value(static_cast<double>(StateEnum::Standing)))
      : CharacterState_Base(std::move(state)) {}
  void update(IStateEntity& e) override;
};

class CharacterState_Running : public CharacterState_Base {
 public:
  explicit CharacterState_Running(Value state = Value(static_cast<double>(StateEnum::Running)))
      : CharacterState_Base(std::move(state)) {}
  void update(IStateEntity& e) override;
};

class CharacterState_Injured : public CharacterState_Base {
 public:
  explicit CharacterState_Injured(Value state = Value(static_cast<double>(StateEnum::Injured)));
};

}
}
