#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/state/weapon_state_base.h"

namespace lfw {
namespace state {

class WeaponState_OnGround : public WeaponState_Base {
 public:
  explicit WeaponState_OnGround(Value state);
  void update(IStateEntity& e) override;
};

class WeaponState_OnHand : public WeaponState_Base {
 public:
  explicit WeaponState_OnHand(Value state);
};

class WeaponState_Throwing : public WeaponState_Base {
 public:
  explicit WeaponState_Throwing(Value state);
  void update(IStateEntity& e) override;
};

class WeaponState_InTheSky : public WeaponState_Base {
 public:
  explicit WeaponState_InTheSky(Value state);
  void update(IStateEntity& e) override;
};

}
}
