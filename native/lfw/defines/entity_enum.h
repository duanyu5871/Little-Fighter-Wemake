#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"
#include "lfw/defines/hit_flag.h"

namespace lfw {

enum class EntityEnum : int {
  Entity = static_cast<int>(HitFlag::Ohters),
  Fighter = static_cast<int>(HitFlag::Fighter),
  Weapon = static_cast<int>(HitFlag::Weapon),
  Ball = static_cast<int>(HitFlag::Ball),
};

inline const std::vector<EnumNumberEntry>& entity_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
      {u"Entity", static_cast<double>(EntityEnum::Entity)},
      {u"Fighter", static_cast<double>(EntityEnum::Fighter)},
      {u"Weapon", static_cast<double>(EntityEnum::Weapon)},
      {u"Ball", static_cast<double>(EntityEnum::Ball)},
  };
  return e;
}

inline const char16_t* entity_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(EntityEnum::Entity):
      return u"Entity";
    case static_cast<int>(EntityEnum::Fighter):
      return u"Fighter";
    case static_cast<int>(EntityEnum::Weapon):
      return u"Weapon";
    case static_cast<int>(EntityEnum::Ball):
      return u"Ball";
  }
  return nullptr;
}

}
