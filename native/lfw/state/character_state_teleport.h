#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Teleport2NearestEnemy : public CharacterState_Base {
 public:
  explicit CharacterState_Teleport2NearestEnemy(
      Value state = Value(static_cast<double>(StateEnum::TeleportToNearestEnemy)));
};

class CharacterState_Teleport2FarthestAlly : public CharacterState_Base {
 public:
  explicit CharacterState_Teleport2FarthestAlly(
      Value state = Value(static_cast<double>(StateEnum::TeleportToFarthestAlly)));
};

}
}
