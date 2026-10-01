#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace bot_state_enum {

inline constexpr const char16_t* kIdle = u"Idle";
inline constexpr const char16_t* kAvoiding = u"Avoiding";
inline constexpr const char16_t* kChasing = u"Chasing";
inline constexpr const char16_t* kFollowing = u"Following";
inline constexpr const char16_t* kStageEnd = u"StageEnd";
inline constexpr const char16_t* kDead = u"Dead";

}

inline const std::vector<EnumTextEntry>& bot_state_enum_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Idle", bot_state_enum::kIdle},
    {u"Avoiding", bot_state_enum::kAvoiding},
    {u"Chasing", bot_state_enum::kChasing},
    {u"Following", bot_state_enum::kFollowing},
    {u"StageEnd", bot_state_enum::kStageEnd},
    {u"Dead", bot_state_enum::kDead},
  };
  return e;
}

}
