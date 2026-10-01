#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class StateEnum : int {
  _Entity_Base = -1,
  _Character_Base = -2,
  _Weapon_Base = -3,
  _Ball_Base = -4,
  Standing = 0,
  Walking = 1,
  Running = 2,
  Attacking = 3,
  Jump = 4,
  Dash = 5,
  Rowing = 6,
  Defend = 7,
  BrokenDefend = 8,
  Catching = 9,
  Caught = 10,
  Injured = 11,
  Falling = 12,
  Frozen = 13,
  Lying = 14,
  Normal = 15,
  Tired = 16,
  Drink = 17,
  Burning = 18,
  BurnRun = 19,
  LandGoto94 = 100,
  Z_Moveable = 301,
  TeleportToNearestEnemy = 400,
  TeleportToFarthestAlly = 401,
  Weapon_InTheSky = 1000,
  Weapon_OnHand = 1001,
  Weapon_Throwing = 1002,
  Weapon_Rebounding = 1003,
  Weapon_OnGround = 1004,
  HealSelf = 1700,
  HeavyWeapon_InTheSky = 2000,
  HeavyWeapon_OnHand = 2001,
  HeavyWeapon_JustOnGround = 2002,
  HeavyWeapon_OnGround = 2004,
  Ball_Flying = 3000,
  Ball_Hitting = 3001,
  Ball_Hit = 3002,
  Ball_Rebounding = 3003,
  Ball_Disappear = 3004,
  Ball_3005 = 3005,
  Ball_3006 = 3006,
  TransformTo_Min = 8001,
  TransformTo_Max = 8999,
  TurnIntoLouisEX = 9995,
  OLD_LouisCastOff = 9996,
  Message = 9997,
  Gone = 9998,
  Weapon_Brokens = 9999,
  TransformToCatching_Begin = 500,
  TransformToCatching_End = 501,
};

inline const std::vector<EnumNumberEntry>& state_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"_Entity_Base", static_cast<double>(StateEnum::_Entity_Base)},
    {u"_Character_Base", static_cast<double>(StateEnum::_Character_Base)},
    {u"_Weapon_Base", static_cast<double>(StateEnum::_Weapon_Base)},
    {u"_Ball_Base", static_cast<double>(StateEnum::_Ball_Base)},
    {u"Standing", static_cast<double>(StateEnum::Standing)},
    {u"Walking", static_cast<double>(StateEnum::Walking)},
    {u"Running", static_cast<double>(StateEnum::Running)},
    {u"Attacking", static_cast<double>(StateEnum::Attacking)},
    {u"Jump", static_cast<double>(StateEnum::Jump)},
    {u"Dash", static_cast<double>(StateEnum::Dash)},
    {u"Rowing", static_cast<double>(StateEnum::Rowing)},
    {u"Defend", static_cast<double>(StateEnum::Defend)},
    {u"BrokenDefend", static_cast<double>(StateEnum::BrokenDefend)},
    {u"Catching", static_cast<double>(StateEnum::Catching)},
    {u"Caught", static_cast<double>(StateEnum::Caught)},
    {u"Injured", static_cast<double>(StateEnum::Injured)},
    {u"Falling", static_cast<double>(StateEnum::Falling)},
    {u"Frozen", static_cast<double>(StateEnum::Frozen)},
    {u"Lying", static_cast<double>(StateEnum::Lying)},
    {u"Normal", static_cast<double>(StateEnum::Normal)},
    {u"Tired", static_cast<double>(StateEnum::Tired)},
    {u"Drink", static_cast<double>(StateEnum::Drink)},
    {u"Burning", static_cast<double>(StateEnum::Burning)},
    {u"BurnRun", static_cast<double>(StateEnum::BurnRun)},
    {u"LandGoto94", static_cast<double>(StateEnum::LandGoto94)},
    {u"Z_Moveable", static_cast<double>(StateEnum::Z_Moveable)},
    {u"TeleportToNearestEnemy", static_cast<double>(StateEnum::TeleportToNearestEnemy)},
    {u"TeleportToFarthestAlly", static_cast<double>(StateEnum::TeleportToFarthestAlly)},
    {u"Weapon_InTheSky", static_cast<double>(StateEnum::Weapon_InTheSky)},
    {u"Weapon_OnHand", static_cast<double>(StateEnum::Weapon_OnHand)},
    {u"Weapon_Throwing", static_cast<double>(StateEnum::Weapon_Throwing)},
    {u"Weapon_Rebounding", static_cast<double>(StateEnum::Weapon_Rebounding)},
    {u"Weapon_OnGround", static_cast<double>(StateEnum::Weapon_OnGround)},
    {u"HealSelf", static_cast<double>(StateEnum::HealSelf)},
    {u"HeavyWeapon_InTheSky", static_cast<double>(StateEnum::HeavyWeapon_InTheSky)},
    {u"HeavyWeapon_OnHand", static_cast<double>(StateEnum::HeavyWeapon_OnHand)},
    {u"HeavyWeapon_JustOnGround", static_cast<double>(StateEnum::HeavyWeapon_JustOnGround)},
    {u"HeavyWeapon_OnGround", static_cast<double>(StateEnum::HeavyWeapon_OnGround)},
    {u"Ball_Flying", static_cast<double>(StateEnum::Ball_Flying)},
    {u"Ball_Hitting", static_cast<double>(StateEnum::Ball_Hitting)},
    {u"Ball_Hit", static_cast<double>(StateEnum::Ball_Hit)},
    {u"Ball_Rebounding", static_cast<double>(StateEnum::Ball_Rebounding)},
    {u"Ball_Disappear", static_cast<double>(StateEnum::Ball_Disappear)},
    {u"Ball_3005", static_cast<double>(StateEnum::Ball_3005)},
    {u"Ball_3006", static_cast<double>(StateEnum::Ball_3006)},
    {u"TransformTo_Min", static_cast<double>(StateEnum::TransformTo_Min)},
    {u"TransformTo_Max", static_cast<double>(StateEnum::TransformTo_Max)},
    {u"TurnIntoLouisEX", static_cast<double>(StateEnum::TurnIntoLouisEX)},
    {u"OLD_LouisCastOff", static_cast<double>(StateEnum::OLD_LouisCastOff)},
    {u"Message", static_cast<double>(StateEnum::Message)},
    {u"Gone", static_cast<double>(StateEnum::Gone)},
    {u"Weapon_Brokens", static_cast<double>(StateEnum::Weapon_Brokens)},
    {u"TransformToCatching_Begin", static_cast<double>(StateEnum::TransformToCatching_Begin)},
    {u"TransformToCatching_End", static_cast<double>(StateEnum::TransformToCatching_End)},
  };
  return e;
}

inline const char16_t* state_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(StateEnum::_Ball_Base): return u"_Ball_Base";
    case static_cast<int>(StateEnum::_Weapon_Base): return u"_Weapon_Base";
    case static_cast<int>(StateEnum::_Character_Base): return u"_Character_Base";
    case static_cast<int>(StateEnum::_Entity_Base): return u"_Entity_Base";
    case static_cast<int>(StateEnum::Standing): return u"Standing";
    case static_cast<int>(StateEnum::Walking): return u"Walking";
    case static_cast<int>(StateEnum::Running): return u"Running";
    case static_cast<int>(StateEnum::Attacking): return u"Attacking";
    case static_cast<int>(StateEnum::Jump): return u"Jump";
    case static_cast<int>(StateEnum::Dash): return u"Dash";
    case static_cast<int>(StateEnum::Rowing): return u"Rowing";
    case static_cast<int>(StateEnum::Defend): return u"Defend";
    case static_cast<int>(StateEnum::BrokenDefend): return u"BrokenDefend";
    case static_cast<int>(StateEnum::Catching): return u"Catching";
    case static_cast<int>(StateEnum::Caught): return u"Caught";
    case static_cast<int>(StateEnum::Injured): return u"Injured";
    case static_cast<int>(StateEnum::Falling): return u"Falling";
    case static_cast<int>(StateEnum::Frozen): return u"Frozen";
    case static_cast<int>(StateEnum::Lying): return u"Lying";
    case static_cast<int>(StateEnum::Normal): return u"Normal";
    case static_cast<int>(StateEnum::Tired): return u"Tired";
    case static_cast<int>(StateEnum::Drink): return u"Drink";
    case static_cast<int>(StateEnum::Burning): return u"Burning";
    case static_cast<int>(StateEnum::BurnRun): return u"BurnRun";
    case static_cast<int>(StateEnum::LandGoto94): return u"LandGoto94";
    case static_cast<int>(StateEnum::Z_Moveable): return u"Z_Moveable";
    case static_cast<int>(StateEnum::TeleportToNearestEnemy): return u"TeleportToNearestEnemy";
    case static_cast<int>(StateEnum::TeleportToFarthestAlly): return u"TeleportToFarthestAlly";
    case static_cast<int>(StateEnum::TransformToCatching_Begin): return u"TransformToCatching_Begin";
    case static_cast<int>(StateEnum::TransformToCatching_End): return u"TransformToCatching_End";
    case static_cast<int>(StateEnum::Weapon_InTheSky): return u"Weapon_InTheSky";
    case static_cast<int>(StateEnum::Weapon_OnHand): return u"Weapon_OnHand";
    case static_cast<int>(StateEnum::Weapon_Throwing): return u"Weapon_Throwing";
    case static_cast<int>(StateEnum::Weapon_Rebounding): return u"Weapon_Rebounding";
    case static_cast<int>(StateEnum::Weapon_OnGround): return u"Weapon_OnGround";
    case static_cast<int>(StateEnum::HealSelf): return u"HealSelf";
    case static_cast<int>(StateEnum::HeavyWeapon_InTheSky): return u"HeavyWeapon_InTheSky";
    case static_cast<int>(StateEnum::HeavyWeapon_OnHand): return u"HeavyWeapon_OnHand";
    case static_cast<int>(StateEnum::HeavyWeapon_JustOnGround): return u"HeavyWeapon_JustOnGround";
    case static_cast<int>(StateEnum::HeavyWeapon_OnGround): return u"HeavyWeapon_OnGround";
    case static_cast<int>(StateEnum::Ball_Flying): return u"Ball_Flying";
    case static_cast<int>(StateEnum::Ball_Hitting): return u"Ball_Hitting";
    case static_cast<int>(StateEnum::Ball_Hit): return u"Ball_Hit";
    case static_cast<int>(StateEnum::Ball_Rebounding): return u"Ball_Rebounding";
    case static_cast<int>(StateEnum::Ball_Disappear): return u"Ball_Disappear";
    case static_cast<int>(StateEnum::Ball_3005): return u"Ball_3005";
    case static_cast<int>(StateEnum::Ball_3006): return u"Ball_3006";
    case static_cast<int>(StateEnum::TransformTo_Min): return u"TransformTo_Min";
    case static_cast<int>(StateEnum::TransformTo_Max): return u"TransformTo_Max";
    case static_cast<int>(StateEnum::TurnIntoLouisEX): return u"TurnIntoLouisEX";
    case static_cast<int>(StateEnum::OLD_LouisCastOff): return u"OLD_LouisCastOff";
    case static_cast<int>(StateEnum::Message): return u"Message";
    case static_cast<int>(StateEnum::Gone): return u"Gone";
    case static_cast<int>(StateEnum::Weapon_Brokens): return u"Weapon_Brokens";
  }
  return nullptr;
}

}
