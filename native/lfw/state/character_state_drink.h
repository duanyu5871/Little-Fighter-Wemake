#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Drink : public CharacterState_Base {
 public:
  explicit CharacterState_Drink(Value state = Value(static_cast<double>(StateEnum::Drink)));
  void update(IStateEntity& e) override;
};

}
}
