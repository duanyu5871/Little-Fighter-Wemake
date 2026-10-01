#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace stage_val {

inline constexpr const char16_t* kEnemiesCleared = u"enemies_cleared";
inline constexpr const char16_t* kDialogCleared = u"dialog_cleared";
inline constexpr const char16_t* kCurPhaseTime = u"cur_phase_time";
inline constexpr const char16_t* kCurDialogTime = u"cur_dialog_time";
inline constexpr const char16_t* kPressAttack = u"press_attack";
inline constexpr const char16_t* kPressJump = u"press_jump";
inline constexpr const char16_t* kPressDefend = u"press_defend";
inline constexpr const char16_t* kPressUp = u"press_up";
inline constexpr const char16_t* kPressDown = u"press_down";
inline constexpr const char16_t* kPressLeft = u"press_left";
inline constexpr const char16_t* kPressRight = u"press_right";
inline constexpr const char16_t* kBroadcast = u"broadcast";

}

inline const std::vector<EnumTextEntry>& stage_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"EnemiesCleared", stage_val::kEnemiesCleared},
    {u"DialogCleared", stage_val::kDialogCleared},
    {u"CurPhaseTime", stage_val::kCurPhaseTime},
    {u"CurDialogTime", stage_val::kCurDialogTime},
    {u"PressAttack", stage_val::kPressAttack},
    {u"PressJump", stage_val::kPressJump},
    {u"PressDefend", stage_val::kPressDefend},
    {u"PressUp", stage_val::kPressUp},
    {u"PressDown", stage_val::kPressDown},
    {u"PressLeft", stage_val::kPressLeft},
    {u"PressRight", stage_val::kPressRight},
    {u"Broadcast", stage_val::kBroadcast},
  };
  return e;
}

}
