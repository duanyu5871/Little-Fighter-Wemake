#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace frame_id {

inline constexpr const char16_t* kNone = u"";
inline constexpr const char16_t* kAuto = u"auto";
inline constexpr const char16_t* kSelf = u"self";
inline constexpr const char16_t* kGone = u"gone";
inline constexpr const char16_t* kInvisible_Min = u"1100";
inline constexpr const char16_t* kInvisible_Max = u"1299";
inline constexpr const char16_t* kRespawn = u"respawn";

}

inline const std::vector<EnumTextEntry>& frame_id_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"None", frame_id::kNone},
    {u"Auto", frame_id::kAuto},
    {u"Self", frame_id::kSelf},
    {u"Gone", frame_id::kGone},
    {u"Invisible_Min", frame_id::kInvisible_Min},
    {u"Invisible_Max", frame_id::kInvisible_Max},
    {u"Respawn", frame_id::kRespawn},
  };
  return e;
}

}
