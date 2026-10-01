#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class HitFlag : int {
  Enemy = 0x01,
  Ally = 0x02,
  Ohters = 0x04,
  Fighter = 0x08,
  Weapon = 0x10,
  Ball = 0x20,
  Dead = 0x80,
  Both = Ally | Enemy,
  AllType = Ohters | Fighter | Weapon | Ball,
  AllEnemy = AllType | Enemy,
  AllAlly = AllType | Ally,
  AllBoth = AllType | Both,
  EnemyFighter = Enemy | Fighter,
  EnemyWeapon = Enemy | Weapon,
  EnemyBall = Enemy | Ball,
  AllyFighter = Ally | Fighter,
  AllyWeapon = Ally | Weapon,
  AllyBall = Ally | Ball,
};

inline const std::vector<EnumNumberEntry>& hit_flag_entries() {
  static const std::vector<EnumNumberEntry> e = {
      {u"Enemy", static_cast<double>(HitFlag::Enemy)},
      {u"Ally", static_cast<double>(HitFlag::Ally)},
      {u"Ohters", static_cast<double>(HitFlag::Ohters)},
      {u"Fighter", static_cast<double>(HitFlag::Fighter)},
      {u"Weapon", static_cast<double>(HitFlag::Weapon)},
      {u"Ball", static_cast<double>(HitFlag::Ball)},
      {u"Dead", static_cast<double>(HitFlag::Dead)},
      {u"Both", static_cast<double>(HitFlag::Both)},
      {u"AllType", static_cast<double>(HitFlag::AllType)},
      {u"AllEnemy", static_cast<double>(HitFlag::AllEnemy)},
      {u"AllAlly", static_cast<double>(HitFlag::AllAlly)},
      {u"AllBoth", static_cast<double>(HitFlag::AllBoth)},
      {u"EnemyFighter", static_cast<double>(HitFlag::EnemyFighter)},
      {u"EnemyWeapon", static_cast<double>(HitFlag::EnemyWeapon)},
      {u"EnemyBall", static_cast<double>(HitFlag::EnemyBall)},
      {u"AllyFighter", static_cast<double>(HitFlag::AllyFighter)},
      {u"AllyWeapon", static_cast<double>(HitFlag::AllyWeapon)},
      {u"AllyBall", static_cast<double>(HitFlag::AllyBall)},
  };
  return e;
}

inline const char16_t* hit_flag_name_of(int v) {
  switch (v) {
    case static_cast<int>(HitFlag::Enemy):
      return u"Enemy";
    case static_cast<int>(HitFlag::Ally):
      return u"Ally";
    case static_cast<int>(HitFlag::Both):
      return u"Both";
    case static_cast<int>(HitFlag::Ohters):
      return u"Ohters";
    case static_cast<int>(HitFlag::Fighter):
      return u"Fighter";
    case static_cast<int>(HitFlag::EnemyFighter):
      return u"EnemyFighter";
    case static_cast<int>(HitFlag::AllyFighter):
      return u"AllyFighter";
    case static_cast<int>(HitFlag::Weapon):
      return u"Weapon";
    case static_cast<int>(HitFlag::EnemyWeapon):
      return u"EnemyWeapon";
    case static_cast<int>(HitFlag::AllyWeapon):
      return u"AllyWeapon";
    case static_cast<int>(HitFlag::Ball):
      return u"Ball";
    case static_cast<int>(HitFlag::EnemyBall):
      return u"EnemyBall";
    case static_cast<int>(HitFlag::AllyBall):
      return u"AllyBall";
    case static_cast<int>(HitFlag::AllType):
      return u"AllType";
    case static_cast<int>(HitFlag::AllEnemy):
      return u"AllEnemy";
    case static_cast<int>(HitFlag::AllAlly):
      return u"AllAlly";
    case static_cast<int>(HitFlag::AllBoth):
      return u"AllBoth";
    case static_cast<int>(HitFlag::Dead):
      return u"Dead";
  }
  return nullptr;
}

}
