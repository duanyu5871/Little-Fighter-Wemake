#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace bot_val {

inline constexpr const char16_t* kDesire = u"desire";
inline constexpr const char16_t* kBotState = u"bot_status";
inline constexpr const char16_t* kEnemyY = u"enemy_y";
inline constexpr const char16_t* kEnemyDiffY = u"enemy_diff_y";
inline constexpr const char16_t* kEnemyX = u"enemy_x";
inline constexpr const char16_t* kEnemyDiffX = u"enemy_diff_x";
inline constexpr const char16_t* kEnemyState = u"enemy_state";
inline constexpr const char16_t* kSafe = u"safe";
inline constexpr const char16_t* kEnemyOutOfRange = u"en_oor";

}

inline const std::vector<EnumTextEntry>& bot_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Desire", bot_val::kDesire},
    {u"BotState", bot_val::kBotState},
    {u"EnemyY", bot_val::kEnemyY},
    {u"EnemyDiffY", bot_val::kEnemyDiffY},
    {u"EnemyX", bot_val::kEnemyX},
    {u"EnemyDiffX", bot_val::kEnemyDiffX},
    {u"EnemyState", bot_val::kEnemyState},
    {u"Safe", bot_val::kSafe},
    {u"EnemyOutOfRange", bot_val::kEnemyOutOfRange},
  };
  return e;
}

}
