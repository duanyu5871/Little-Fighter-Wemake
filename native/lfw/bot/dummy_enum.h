#pragma once

#include <string>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {
namespace bot {

namespace dummy_enum {

inline constexpr const char16_t* kNone = u"";
inline constexpr const char16_t* kLockAtMid_Stand = u"1";
inline constexpr const char16_t* kLockAtMid_Defend = u"2";
inline constexpr const char16_t* kLockAtMid_RowingWhenFalling = u"3";
inline constexpr const char16_t* kLockAtMid_JumpAndRowingWhenFalling = u"4";
inline constexpr const char16_t* kAvoidEnemyAllTheTime = u"5";
inline constexpr const char16_t* kLockAtMid_dUa = u"6";
inline constexpr const char16_t* kLockAtMid_dUj = u"7";
inline constexpr const char16_t* kLockAtMid_dDa = u"8";
inline constexpr const char16_t* kLockAtMid_dDj = u"9";
inline constexpr const char16_t* kLockAtMid_dLa = u"10";
inline constexpr const char16_t* kLockAtMid_dLj = u"11";
inline constexpr const char16_t* kLockAtMid_dRa = u"12";
inline constexpr const char16_t* kLockAtMid_dRj = u"13";
inline constexpr const char16_t* kLockAtMid_dja = u"14";
inline constexpr const char16_t* kLockAtMid_dUa_auto = u"15";
inline constexpr const char16_t* kLockAtMid_dUj_auto = u"16";
inline constexpr const char16_t* kLockAtMid_dDa_auto = u"17";
inline constexpr const char16_t* kLockAtMid_dDj_auto = u"18";
inline constexpr const char16_t* kLockAtMid_dLa_auto = u"18";
inline constexpr const char16_t* kLockAtMid_dLj_auto = u"19";
inline constexpr const char16_t* kLockAtMid_dRa_auto = u"20";
inline constexpr const char16_t* kLockAtMid_dRj_auto = u"21";
inline constexpr const char16_t* kLockAtMid_dja_auto = u"22";

}

const std::vector<EnumTextEntry>& dummy_enum_entries();

const std::vector<std::u16string>& dummy_updater_ids();

}
}
