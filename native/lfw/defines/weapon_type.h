#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class WeaponEnum : int {
  None = 0,
  Stick = 1,
  Heavy = 2,
  Knife = 3,
  Baseball = 4,
  Drink = 5,
};

inline const std::vector<EnumNumberEntry>& weapon_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"None", static_cast<double>(WeaponEnum::None)},
    {u"Stick", static_cast<double>(WeaponEnum::Stick)},
    {u"Heavy", static_cast<double>(WeaponEnum::Heavy)},
    {u"Knife", static_cast<double>(WeaponEnum::Knife)},
    {u"Baseball", static_cast<double>(WeaponEnum::Baseball)},
    {u"Drink", static_cast<double>(WeaponEnum::Drink)},
  };
  return e;
}

inline const char16_t* weapon_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(WeaponEnum::None): return u"None";
    case static_cast<int>(WeaponEnum::Stick): return u"Stick";
    case static_cast<int>(WeaponEnum::Heavy): return u"Heavy";
    case static_cast<int>(WeaponEnum::Knife): return u"Knife";
    case static_cast<int>(WeaponEnum::Baseball): return u"Baseball";
    case static_cast<int>(WeaponEnum::Drink): return u"Drink";
  }
  return nullptr;
}

}
