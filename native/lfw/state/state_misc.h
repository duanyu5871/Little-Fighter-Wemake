#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/state_base.h"

namespace lfw {
namespace state {

class State_WeaponBroken : public State_Base {
 public:
  explicit State_WeaponBroken(Value state = Value(static_cast<double>(StateEnum::Weapon_Brokens)));
};

class State_TransformToCatching : public State_Base {
 public:
  explicit State_TransformToCatching(
      Value state = Value(static_cast<double>(StateEnum::TransformToCatching_End)));
  void update(IStateEntity& e) override;
};

class CharacterState_TransformToLouisEX : public State_Base {
 public:
  explicit CharacterState_TransformToLouisEX(
      Value state = Value(static_cast<double>(StateEnum::TurnIntoLouisEX)));
};

class State_TransformTo8XXX : public State_Base {
 public:
  using State_Base::State_Base;
  void leave(IStateEntity& e, const Value& next_frame) override;
};

class BallState_Base : public State_Base {
 public:
  explicit BallState_Base(Value state);
};

}
}
