#pragma once

#include <string>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace team_enum {

inline constexpr const char16_t* kIndependent = u"";
inline constexpr const char16_t* kTeam_1 = u"1";
inline constexpr const char16_t* kTeam_2 = u"2";
inline constexpr const char16_t* kTeam_3 = u"3";
inline constexpr const char16_t* kTeam_4 = u"4";
inline constexpr const char16_t* kTeam_5 = u"5";
inline constexpr const char16_t* kTeam_6 = u"6";
inline constexpr const char16_t* kTeam_7 = u"7";
inline constexpr const char16_t* kTeam_8 = u"8";
inline constexpr const char16_t* kMax = u"8";

}

inline bool is_independent(const std::u16string& team) { return team.size() != 1; }

inline const std::vector<EnumTextEntry>& team_enum_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Independent", team_enum::kIndependent},
    {u"Team_1", team_enum::kTeam_1},
    {u"Team_2", team_enum::kTeam_2},
    {u"Team_3", team_enum::kTeam_3},
    {u"Team_4", team_enum::kTeam_4},
    {u"Team_5", team_enum::kTeam_5},
    {u"Team_6", team_enum::kTeam_6},
    {u"Team_7", team_enum::kTeam_7},
    {u"Team_8", team_enum::kTeam_8},
    {u"Max", team_enum::kMax},
  };
  return e;
}

}
