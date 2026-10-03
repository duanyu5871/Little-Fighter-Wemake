#pragma once

#include <type_traits>

#include "lfw/state/character_state_base.h"
#include "lfw/state/character_state_basic.h"
#include "lfw/state/character_state_burning.h"
#include "lfw/state/character_state_caught_rowing.h"
#include "lfw/state/character_state_dash.h"
#include "lfw/state/character_state_drink.h"
#include "lfw/state/character_state_falling.h"
#include "lfw/state/character_state_jump.h"
#include "lfw/state/character_state_lying.h"
#include "lfw/state/character_state_teleport.h"
#include "lfw/state/character_state_walking.h"
#include "lfw/state/state_base.h"
#include "lfw/state/state_base_proxy.h"
#include "lfw/state/state_burning.h"
#include "lfw/state/state_misc.h"
#include "lfw/state/weapon_state_base.h"
#include "lfw/state/weapon_state_misc.h"

namespace lfw {
namespace state {

// TS reads `constructor.name` when dumping the state table. RTTI is off (`/GR-`),
// so every class that can be registered needs an explicit name here; the primary
// template stays undefined so a forgotten class fails to compile.
template <typename T>
struct state_name_of;

template <>
struct state_name_of<State_Base> {
  static constexpr const char* value = "State_Base";
};
template <>
struct state_name_of<CharacterState_Base> {
  static constexpr const char* value = "CharacterState_Base";
};
template <>
struct state_name_of<BallState_Base> {
  static constexpr const char* value = "BallState_Base";
};
template <>
struct state_name_of<WeaponState_Base> {
  static constexpr const char* value = "WeaponState_Base";
};
template <>
struct state_name_of<StateBase_Proxy> {
  static constexpr const char* value = "StateBase_Proxy";
};
template <>
struct state_name_of<State_15> {
  static constexpr const char* value = "State_15";
};
template <>
struct state_name_of<State_Frozen> {
  static constexpr const char* value = "State_Frozen";
};
template <>
struct state_name_of<State_Burning> {
  static constexpr const char* value = "State_Burning";
};
template <>
struct state_name_of<State_WeaponBroken> {
  static constexpr const char* value = "State_WeaponBroken";
};
template <>
struct state_name_of<State_TransformToCatching> {
  static constexpr const char* value = "State_TransformToCatching";
};
template <>
struct state_name_of<State_TransformTo8XXX> {
  static constexpr const char* value = "State_TransformTo8XXX";
};
template <>
struct state_name_of<CharacterState_Standing> {
  static constexpr const char* value = "CharacterState_Standing";
};
template <>
struct state_name_of<CharacterState_Walking> {
  static constexpr const char* value = "CharacterState_Walking";
};
template <>
struct state_name_of<CharacterState_Running> {
  static constexpr const char* value = "CharacterState_Running";
};
template <>
struct state_name_of<CharacterState_Jump> {
  static constexpr const char* value = "CharacterState_Jump";
};
template <>
struct state_name_of<CharacterState_Dash> {
  static constexpr const char* value = "CharacterState_Dash";
};
template <>
struct state_name_of<CharacterState_Falling> {
  static constexpr const char* value = "CharacterState_Falling";
};
template <>
struct state_name_of<CharacterState_Lying> {
  static constexpr const char* value = "CharacterState_Lying";
};
template <>
struct state_name_of<CharacterState_Injured> {
  static constexpr const char* value = "CharacterState_Injured";
};
template <>
struct state_name_of<CharacterState_Caught> {
  static constexpr const char* value = "CharacterState_Caught";
};
template <>
struct state_name_of<CharacterState_Rowing> {
  static constexpr const char* value = "CharacterState_Rowing";
};
template <>
struct state_name_of<CharacterState_Drink> {
  static constexpr const char* value = "CharacterState_Drink";
};
template <>
struct state_name_of<CharacterState_Burning> {
  static constexpr const char* value = "CharacterState_Burning";
};
template <>
struct state_name_of<CharacterState_Teleport2NearestEnemy> {
  static constexpr const char* value = "CharacterState_Teleport2NearestEnemy";
};
template <>
struct state_name_of<CharacterState_Teleport2FarthestAlly> {
  static constexpr const char* value = "CharacterState_Teleport2FarthestAlly";
};
template <>
struct state_name_of<CharacterState_TransformToLouisEX> {
  static constexpr const char* value = "CharacterState_TransformToLouisEX";
};
template <>
struct state_name_of<WeaponState_InTheSky> {
  static constexpr const char* value = "WeaponState_InTheSky";
};
template <>
struct state_name_of<WeaponState_OnGround> {
  static constexpr const char* value = "WeaponState_OnGround";
};
template <>
struct state_name_of<WeaponState_OnHand> {
  static constexpr const char* value = "WeaponState_OnHand";
};
template <>
struct state_name_of<WeaponState_Throwing> {
  static constexpr const char* value = "WeaponState_Throwing";
};

template <typename T>
constexpr const char* state_name() {
  return state_name_of<std::decay_t<T>>::value;
}

// The names above are pure ASCII, so a byte-wise widening is enough.
inline std::u16string ascii_u16(const char* s) {
  std::u16string out;
  for (const char* p = s; p != nullptr && *p != '\0'; ++p) {
    out.push_back(static_cast<char16_t>(static_cast<unsigned char>(*p)));
  }
  return out;
}

template <typename T>
std::u16string state_name_u16() {
  return ascii_u16(state_name<T>());
}

}
}
