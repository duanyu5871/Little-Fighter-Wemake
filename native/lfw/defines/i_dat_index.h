#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace dat_type_enum {

inline constexpr const char16_t* kInvalid = u"";
inline constexpr const char16_t* kFighter = u"0";
inline constexpr const char16_t* kWeaponA = u"1";
inline constexpr const char16_t* kWeaponB = u"2";
inline constexpr const char16_t* kBall = u"3";
inline constexpr const char16_t* kWeaponC = u"4";
inline constexpr const char16_t* kCriminal = u"5";
inline constexpr const char16_t* kWeaponD = u"6";
inline constexpr const char16_t* kBackground = u"bg";
inline constexpr const char16_t* kStage = u"stage";
inline constexpr const char16_t* kBot = u"bot";

}

inline const std::vector<EnumTextEntry>& dat_type_enum_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Invalid", dat_type_enum::kInvalid},
    {u"Fighter", dat_type_enum::kFighter},
    {u"WeaponA", dat_type_enum::kWeaponA},
    {u"WeaponB", dat_type_enum::kWeaponB},
    {u"Ball", dat_type_enum::kBall},
    {u"WeaponC", dat_type_enum::kWeaponC},
    {u"Criminal", dat_type_enum::kCriminal},
    {u"WeaponD", dat_type_enum::kWeaponD},
    {u"Background", dat_type_enum::kBackground},
    {u"Stage", dat_type_enum::kStage},
    {u"Bot", dat_type_enum::kBot},
  };
  return e;
}

}
