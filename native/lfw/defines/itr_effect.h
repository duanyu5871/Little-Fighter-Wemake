#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class ItrEffect : int {
  Normal = 0,
  Sharp = 1,
  Fire = 2,
  Ice = 3,
  Through = 4,
  None = 5,
  MFire1 = 20,
  MFire2 = 21,
  FireExplosion = 22,
  Explosion = 23,
  Ice2 = 30,
  Ignore = 10000,
};

inline const std::vector<EnumNumberEntry>& itr_effect_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Normal", static_cast<double>(ItrEffect::Normal)},
    {u"Sharp", static_cast<double>(ItrEffect::Sharp)},
    {u"Fire", static_cast<double>(ItrEffect::Fire)},
    {u"Ice", static_cast<double>(ItrEffect::Ice)},
    {u"Through", static_cast<double>(ItrEffect::Through)},
    {u"None", static_cast<double>(ItrEffect::None)},
    {u"MFire1", static_cast<double>(ItrEffect::MFire1)},
    {u"MFire2", static_cast<double>(ItrEffect::MFire2)},
    {u"FireExplosion", static_cast<double>(ItrEffect::FireExplosion)},
    {u"Explosion", static_cast<double>(ItrEffect::Explosion)},
    {u"Ice2", static_cast<double>(ItrEffect::Ice2)},
    {u"Ignore", static_cast<double>(ItrEffect::Ignore)},
  };
  return e;
}

inline const char16_t* itr_effect_name_of(int v) {
  switch (v) {
    case static_cast<int>(ItrEffect::Normal): return u"Normal";
    case static_cast<int>(ItrEffect::Sharp): return u"Sharp";
    case static_cast<int>(ItrEffect::Fire): return u"Fire";
    case static_cast<int>(ItrEffect::Ice): return u"Ice";
    case static_cast<int>(ItrEffect::Through): return u"Through";
    case static_cast<int>(ItrEffect::None): return u"None";
    case static_cast<int>(ItrEffect::MFire1): return u"MFire1";
    case static_cast<int>(ItrEffect::MFire2): return u"MFire2";
    case static_cast<int>(ItrEffect::FireExplosion): return u"FireExplosion";
    case static_cast<int>(ItrEffect::Explosion): return u"Explosion";
    case static_cast<int>(ItrEffect::Ice2): return u"Ice2";
    case static_cast<int>(ItrEffect::Ignore): return u"Ignore";
  }
  return nullptr;
}

}
