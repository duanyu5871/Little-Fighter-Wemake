#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class ItrKind : int {
  Normal = 0,
  Catch = 1,
  Pick = 2,
  ForceCatch = 3,
  CharacterThrew = 4,
  WeaponSwing = 5,
  SuperPunchMe = 6,
  PickSecretly = 7,
  Heal = 8,
  JohnShield = 9,
  MagicFlute = 10,
  MagicFlute2 = 11,
  Block = 14,
  Whirlwind = 15,
  Freeze = 16,
};

inline const std::vector<EnumNumberEntry>& itr_kind_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Normal", static_cast<double>(ItrKind::Normal)},
    {u"Catch", static_cast<double>(ItrKind::Catch)},
    {u"Pick", static_cast<double>(ItrKind::Pick)},
    {u"ForceCatch", static_cast<double>(ItrKind::ForceCatch)},
    {u"CharacterThrew", static_cast<double>(ItrKind::CharacterThrew)},
    {u"WeaponSwing", static_cast<double>(ItrKind::WeaponSwing)},
    {u"SuperPunchMe", static_cast<double>(ItrKind::SuperPunchMe)},
    {u"PickSecretly", static_cast<double>(ItrKind::PickSecretly)},
    {u"Heal", static_cast<double>(ItrKind::Heal)},
    {u"JohnShield", static_cast<double>(ItrKind::JohnShield)},
    {u"MagicFlute", static_cast<double>(ItrKind::MagicFlute)},
    {u"MagicFlute2", static_cast<double>(ItrKind::MagicFlute2)},
    {u"Block", static_cast<double>(ItrKind::Block)},
    {u"Whirlwind", static_cast<double>(ItrKind::Whirlwind)},
    {u"Freeze", static_cast<double>(ItrKind::Freeze)},
  };
  return e;
}

// `ATTCKING_ITR_KINDS`（`defines/ItrKind.ts`）：`BotController.should_defend` 用它筛
// 「这一条 itr 算不算攻击判定」。成员只做 `some(v => itr.kind === v)` 比较。
inline const std::vector<double>& attcking_itr_kinds() {
  static const std::vector<double> v = {
    static_cast<double>(ItrKind::Normal),
    static_cast<double>(ItrKind::JohnShield),
    static_cast<double>(ItrKind::WeaponSwing),
  };
  return v;
}

inline const char16_t* itr_kind_name_of(int v) {
  switch (v) {
    case static_cast<int>(ItrKind::Normal): return u"Normal";
    case static_cast<int>(ItrKind::Catch): return u"Catch";
    case static_cast<int>(ItrKind::Pick): return u"Pick";
    case static_cast<int>(ItrKind::ForceCatch): return u"ForceCatch";
    case static_cast<int>(ItrKind::CharacterThrew): return u"CharacterThrew";
    case static_cast<int>(ItrKind::WeaponSwing): return u"WeaponSwing";
    case static_cast<int>(ItrKind::SuperPunchMe): return u"SuperPunchMe";
    case static_cast<int>(ItrKind::PickSecretly): return u"PickSecretly";
    case static_cast<int>(ItrKind::Heal): return u"Heal";
    case static_cast<int>(ItrKind::JohnShield): return u"JohnShield";
    case static_cast<int>(ItrKind::MagicFlute): return u"MagicFlute";
    case static_cast<int>(ItrKind::MagicFlute2): return u"MagicFlute2";
    case static_cast<int>(ItrKind::Block): return u"Block";
    case static_cast<int>(ItrKind::Whirlwind): return u"Whirlwind";
    case static_cast<int>(ItrKind::Freeze): return u"Freeze";
  }
  return nullptr;
}

}
