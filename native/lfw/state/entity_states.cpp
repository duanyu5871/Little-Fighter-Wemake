#include "lfw/state/entity_states.h"

#include <vector>

#include "lfw/defines/state_enum.h"

namespace lfw {
namespace state {
namespace {

double se(StateEnum v) { return static_cast<double>(v); }

States build() {
  States states;
  states.set_in_range<State_TransformTo8XXX>(se(StateEnum::TransformTo_Min),
                                             se(StateEnum::TransformTo_Max));
  states.set_all_of<StateBase_Proxy>({
      Value(se(StateEnum::_Ball_Base)),
      Value(se(StateEnum::Ball_3005)),
      Value(se(StateEnum::Ball_3006)),
      Value(se(StateEnum::Ball_Disappear)),
      Value(se(StateEnum::Ball_Flying)),
      Value(se(StateEnum::Ball_Hit)),
      Value(se(StateEnum::Ball_Hitting)),
  });
  states.add<State_WeaponBroken>();
  states.add<State_TransformToCatching>();
  states.add<WeaponState_Base>(Value(se(StateEnum::Weapon_Rebounding)));
  states.add<WeaponState_InTheSky>(Value(se(StateEnum::Weapon_InTheSky)));
  states.add<WeaponState_OnGround>(Value(se(StateEnum::Weapon_OnGround)));
  states.add<WeaponState_OnHand>(Value(se(StateEnum::Weapon_OnHand)));
  states.add<WeaponState_Throwing>(Value(se(StateEnum::Weapon_Throwing)));
  states.add<WeaponState_InTheSky>(Value(se(StateEnum::HeavyWeapon_InTheSky)));
  states.add<WeaponState_OnGround>(Value(se(StateEnum::HeavyWeapon_OnGround)));
  states.add<WeaponState_OnHand>(Value(se(StateEnum::HeavyWeapon_OnHand)));
  states.add<WeaponState_Throwing>(Value(se(StateEnum::HeavyWeapon_JustOnGround)));
  states.add<State_Base>(Value(se(StateEnum::_Entity_Base)));
  states.add<WeaponState_Base>(Value(se(StateEnum::_Weapon_Base)));
  states.add<CharacterState_Base>(Value(se(StateEnum::_Character_Base)));
  states.add<CharacterState_Standing>();
  states.add<CharacterState_Walking>();
  states.add<CharacterState_Running>();
  states.add<CharacterState_Jump>();
  states.add<CharacterState_Dash>();
  states.add<CharacterState_Falling>();
  states.add<State_Burning>();
  states.add<State_Frozen>();
  states.add<CharacterState_Lying>();
  states.add<CharacterState_Caught>();
  states.add<CharacterState_Injured>();
  states.add<CharacterState_Injured>(Value(se(StateEnum::Tired)));
  states.add<CharacterState_Base>(Value(se(StateEnum::Z_Moveable)));
  states.add<CharacterState_Teleport2NearestEnemy>();
  states.add<CharacterState_Teleport2FarthestAlly>();
  states.add<CharacterState_TransformToLouisEX>();
  states.add<CharacterState_Rowing>();
  states.add<CharacterState_Drink>();
  states.add<State_15>();
  states.add<StateBase_Proxy>(Value(se(StateEnum::LandGoto94)));
  return states;
}

}

States& entity_states() {
  static States states = build();
  return states;
}

}
}
