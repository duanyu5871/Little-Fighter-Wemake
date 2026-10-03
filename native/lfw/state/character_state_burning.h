#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Burning : public CharacterState_Base {
 public:
  CharacterState_Burning();
  void update(IStateEntity& e) override;
  void leave(IStateEntity& e, const Value& next_frame) override;
};

}
}
