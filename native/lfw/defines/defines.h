#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace built_in_imgs {

inline constexpr const char16_t* kRFACE = u"!sprite/RFACE@4x.png";
inline constexpr const char16_t* kCHARACTER_THUMB = u"sprite/CHARACTER_THUMB.png";

}

inline const std::vector<EnumTextEntry>& built_in_imgs_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"RFACE", built_in_imgs::kRFACE},
    {u"CHARACTER_THUMB", built_in_imgs::kCHARACTER_THUMB},
  };
  return e;
}

namespace built_in_dats {

inline constexpr const char16_t* kSpark = u"data/spark.obj.json5";

}

inline const std::vector<EnumTextEntry>& built_in_dats_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Spark", built_in_dats::kSpark},
  };
  return e;
}

namespace built_in_broadcast {

inline constexpr const char16_t* kResetGPL = u"reset_gpl";
inline constexpr const char16_t* kUpdateRandom = u"update_random";
inline constexpr const char16_t* kStartGame = u"start_game";
inline constexpr const char16_t* kSwitchStage = u"switch_stage";
inline constexpr const char16_t* kSwitchStageR = u"switch_stage_r";
inline constexpr const char16_t* kSwitchBackground = u"switch_background";
inline constexpr const char16_t* kSwitchBackgroundR = u"switch_background_r";
inline constexpr const char16_t* kResetBackground = u"reset_background";

}

inline const std::vector<EnumTextEntry>& built_in_broadcast_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"ResetGPL", built_in_broadcast::kResetGPL},
    {u"UpdateRandom", built_in_broadcast::kUpdateRandom},
    {u"StartGame", built_in_broadcast::kStartGame},
    {u"SwitchStage", built_in_broadcast::kSwitchStage},
    {u"SwitchStageR", built_in_broadcast::kSwitchStageR},
    {u"SwitchBackground", built_in_broadcast::kSwitchBackground},
    {u"SwitchBackgroundR", built_in_broadcast::kSwitchBackgroundR},
    {u"ResetBackground", built_in_broadcast::kResetBackground},
  };
  return e;
}

namespace built_in_sounds {

inline constexpr const char16_t* kCancel = u"cancel";
inline constexpr const char16_t* kEnd = u"end";
inline constexpr const char16_t* kJoin = u"join";
inline constexpr const char16_t* kOk = u"ok";
inline constexpr const char16_t* kPass = u"pass";

}

inline const std::vector<EnumTextEntry>& built_in_sounds_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Cancel", built_in_sounds::kCancel},
    {u"End", built_in_sounds::kEnd},
    {u"Join", built_in_sounds::kJoin},
    {u"Ok", built_in_sounds::kOk},
    {u"Pass", built_in_sounds::kPass},
  };
  return e;
}

}
