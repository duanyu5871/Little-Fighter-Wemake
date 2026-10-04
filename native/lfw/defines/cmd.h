#pragma once

#include <vector>

#include "lfw/defines/cheat_type.h"
#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace cmd {

inline constexpr const char16_t* kF1 = u"f1";
inline constexpr const char16_t* kF2 = u"f2";
inline constexpr const char16_t* kF3 = u"f3";
inline constexpr const char16_t* kF4 = u"f4";
inline constexpr const char16_t* kF5 = u"f5";
inline constexpr const char16_t* kF6 = u"f6";
inline constexpr const char16_t* kF7 = u"f7";
inline constexpr const char16_t* kF8 = u"f8";
inline constexpr const char16_t* kF9 = u"f9";
inline constexpr const char16_t* kF10 = u"f10";
inline constexpr const char16_t* kLF2_NET = cheat_enum::kLF2_NET;
inline constexpr const char16_t* kHERO_FT = cheat_enum::kHERO_FT;
inline constexpr const char16_t* kGIM_INK = cheat_enum::kGIM_INK;
inline constexpr const char16_t* kKILL_ENEMIES = u"KILL_ENEMIES";
inline constexpr const char16_t* kKILL_BOSS = u"KILL_BOSS";
inline constexpr const char16_t* kKILL_SOLIDERS = u"KILL_SOLIDERS";
inline constexpr const char16_t* kKILL_OTHERS = u"KILL_OTHERS";
inline constexpr const char16_t* kSET_PUPPET = u"SET_PUPPET";
inline constexpr const char16_t* kDEL_PUPPET = u"DEL_PUPPET";
inline constexpr const char16_t* kSET_DIFFICULTY = u"SET_DIFFICULTY";
inline constexpr const char16_t* kDIST_CAM = u"DIST_CAM";
inline constexpr const char16_t* kLOCK_CAM = u"LOCK_CAM";
inline constexpr const char16_t* kCHANGE_BG = u"CHANGE_BG";
inline constexpr const char16_t* kCHANGE_STAGE = u"CHANGE_STAGE";
inline constexpr const char16_t* kBGM = u"bgm";
inline constexpr const char16_t* kPAUSE = u"PAUSE";
inline constexpr const char16_t* kSPAWN = u"SPAWN";
inline constexpr const char16_t* kDESPAWN = u"DESPAWN";
inline constexpr const char16_t* kKILL = u"KILL";
inline constexpr const char16_t* kPOINTER_DOWN = u"POINTER_DOWN";
inline constexpr const char16_t* kPOINTER_MOVE = u"POINTER_MOVE";
inline constexpr const char16_t* kPOINTER_UP = u"POINTER_UP";
inline constexpr const char16_t* kPOINTER_CANCEL = u"POINTER_CANCEL";
inline constexpr const char16_t* kPOINTER_LEAVE = u"POINTER_LEAVE";
inline constexpr const char16_t* kPOINTER_ENTER = u"POINTER_ENTER";
inline constexpr const char16_t* kPOINTER_CLICK = u"POINTER_CLICK";
inline constexpr const char16_t* kKEY_EVENT = u"KEY_EVENT";

}

inline const std::vector<EnumTextEntry>& cmd_entries() {
  static const std::vector<EnumTextEntry> e = {
      {u"F1", cmd::kF1},
      {u"F2", cmd::kF2},
      {u"F3", cmd::kF3},
      {u"F4", cmd::kF4},
      {u"F5", cmd::kF5},
      {u"F6", cmd::kF6},
      {u"F7", cmd::kF7},
      {u"F8", cmd::kF8},
      {u"F9", cmd::kF9},
      {u"F10", cmd::kF10},
      {u"LF2_NET", cmd::kLF2_NET},
      {u"HERO_FT", cmd::kHERO_FT},
      {u"GIM_INK", cmd::kGIM_INK},
      {u"KILL_ENEMIES", cmd::kKILL_ENEMIES},
      {u"KILL_BOSS", cmd::kKILL_BOSS},
      {u"KILL_SOLIDERS", cmd::kKILL_SOLIDERS},
      {u"KILL_OTHERS", cmd::kKILL_OTHERS},
      {u"SET_PUPPET", cmd::kSET_PUPPET},
      {u"DEL_PUPPET", cmd::kDEL_PUPPET},
      {u"SET_DIFFICULTY", cmd::kSET_DIFFICULTY},
      {u"DIST_CAM", cmd::kDIST_CAM},
      {u"LOCK_CAM", cmd::kLOCK_CAM},
      {u"CHANGE_BG", cmd::kCHANGE_BG},
      {u"CHANGE_STAGE", cmd::kCHANGE_STAGE},
      {u"BGM", cmd::kBGM},
      {u"PAUSE", cmd::kPAUSE},
      {u"SPAWN", cmd::kSPAWN},
      {u"DESPAWN", cmd::kDESPAWN},
      {u"KILL", cmd::kKILL},
      {u"POINTER_DOWN", cmd::kPOINTER_DOWN},
      {u"POINTER_MOVE", cmd::kPOINTER_MOVE},
      {u"POINTER_UP", cmd::kPOINTER_UP},
      {u"POINTER_CANCEL", cmd::kPOINTER_CANCEL},
      {u"POINTER_LEAVE", cmd::kPOINTER_LEAVE},
      {u"POINTER_ENTER", cmd::kPOINTER_ENTER},
      {u"POINTER_CLICK", cmd::kPOINTER_CLICK},
      {u"KEY_EVENT", cmd::kKEY_EVENT},
  };
  return e;
}

}
