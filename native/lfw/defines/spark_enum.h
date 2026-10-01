#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace spark_enum {

inline constexpr const char16_t* kBrokenDefend = u"broken_defend";
inline constexpr const char16_t* kCriticalHit = u"critical_hit";
inline constexpr const char16_t* kSilentCriticalHit = u"silent_critical_hit";
inline constexpr const char16_t* kDefendHit = u"defend_hit";
inline constexpr const char16_t* kHit = u"hit";
inline constexpr const char16_t* kHitFall = u"hit_fall";
inline constexpr const char16_t* kSilentHit = u"silent_hit";
inline constexpr const char16_t* kBleed = u"bleed";
inline constexpr const char16_t* kBleedFall = u"bleed_fall";
inline constexpr const char16_t* kCriticalBleed = u"critical_bleed";

}

inline const std::vector<EnumTextEntry>& spark_enum_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"BrokenDefend", spark_enum::kBrokenDefend},
    {u"CriticalHit", spark_enum::kCriticalHit},
    {u"SilentCriticalHit", spark_enum::kSilentCriticalHit},
    {u"DefendHit", spark_enum::kDefendHit},
    {u"Hit", spark_enum::kHit},
    {u"HitFall", spark_enum::kHitFall},
    {u"SilentHit", spark_enum::kSilentHit},
    {u"Bleed", spark_enum::kBleed},
    {u"BleedFall", spark_enum::kBleedFall},
    {u"CriticalBleed", spark_enum::kCriticalBleed},
  };
  return e;
}

}
