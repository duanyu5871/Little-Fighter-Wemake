#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace entity_group {

inline constexpr const char16_t* kHidden = u"hidden";
inline constexpr const char16_t* kBoss = u"Boss";
inline constexpr const char16_t* kGaint = u"Giant";
inline constexpr const char16_t* kRegular = u"1000";
inline constexpr const char16_t* k_3000 = u"3000";
inline constexpr const char16_t* kVsWeapon = u"VsWeapon";
inline constexpr const char16_t* kStageWeapon = u"StageWeapon";
inline constexpr const char16_t* kFreezableBall = u"FreezableBall";
inline constexpr const char16_t* kFreezer = u"Freezer";
inline constexpr const char16_t* kDev = u"Dev";

}

inline const std::vector<EnumTextEntry>& entity_group_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Hidden", entity_group::kHidden},
    {u"Boss", entity_group::kBoss},
    {u"Gaint", entity_group::kGaint},
    {u"Regular", entity_group::kRegular},
    {u"_3000", entity_group::k_3000},
    {u"VsWeapon", entity_group::kVsWeapon},
    {u"StageWeapon", entity_group::kStageWeapon},
    {u"FreezableBall", entity_group::kFreezableBall},
    {u"Freezer", entity_group::kFreezer},
    {u"Dev", entity_group::kDev},
  };
  return e;
}

}
