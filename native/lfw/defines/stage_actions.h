#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace stage_actions {

inline constexpr const char16_t* kGoGoGoRight = u"gogogo_right";
inline constexpr const char16_t* kEnterNextPhase = u"enter_next_phase";
inline constexpr const char16_t* kLoopGoGoGoRight = u"loop_gogogo_right";

}

inline const std::vector<EnumTextEntry>& stage_actions_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"GoGoGoRight", stage_actions::kGoGoGoRight},
    {u"EnterNextPhase", stage_actions::kEnterNextPhase},
    {u"LoopGoGoGoRight", stage_actions::kLoopGoGoGoRight},
  };
  return e;
}

}
