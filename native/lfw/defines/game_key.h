#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace gk {

inline constexpr const char16_t* kL = u"L";
inline constexpr const char16_t* kLeft = u"L";
inline constexpr const char16_t* kR = u"R";
inline constexpr const char16_t* kRight = u"R";
inline constexpr const char16_t* kU = u"U";
inline constexpr const char16_t* kUp = u"U";
inline constexpr const char16_t* kD = u"D";
inline constexpr const char16_t* kDown = u"D";
inline constexpr const char16_t* ka = u"a";
inline constexpr const char16_t* kAttack = u"a";
inline constexpr const char16_t* kj = u"j";
inline constexpr const char16_t* kJump = u"j";
inline constexpr const char16_t* kd = u"d";
inline constexpr const char16_t* kDefend = u"d";

}

inline const std::vector<EnumTextEntry>& gk_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"L", gk::kL},
    {u"Left", gk::kLeft},
    {u"R", gk::kR},
    {u"Right", gk::kRight},
    {u"U", gk::kU},
    {u"Up", gk::kUp},
    {u"D", gk::kD},
    {u"Down", gk::kDown},
    {u"a", gk::ka},
    {u"Attack", gk::kAttack},
    {u"j", gk::kj},
    {u"Jump", gk::kJump},
    {u"d", gk::kd},
    {u"Defend", gk::kDefend},
  };
  return e;
}

}
