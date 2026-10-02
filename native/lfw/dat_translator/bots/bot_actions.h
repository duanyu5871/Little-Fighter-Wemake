#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bots_types.h"

namespace lfw {
namespace dat_translator {
namespace bots {

inline constexpr const char16_t* kBallDfaId = u"d>a";
inline constexpr const char16_t* kBallDfjId = u"d>j";
inline constexpr const char16_t* kUppercutDuaId = u"d^a";
inline constexpr const char16_t* kUppercutDujId = u"d^j";
inline constexpr const char16_t* kUppercutDvaId = u"dva";
inline constexpr const char16_t* kExplosionDuaId = u"d^a";
inline constexpr const char16_t* kExplosionDujId = u"d^j";

inline constexpr double kUppercutDuaMinX = -10.0;
inline constexpr double kUppercutDuaMaxX = 120.0;
inline constexpr double kUppercutDvaMinX = -10.0;
inline constexpr double kUppercutDvaMaxX = 120.0;
inline constexpr double kExplosionDuaMinX = -120.0;
inline constexpr double kExplosionDuaMaxX = 120.0;
inline constexpr double kExplosionDuaZLen = 120.0;
inline constexpr double kExplosionDujMinX = -120.0;
inline constexpr double kExplosionDujMaxX = 120.0;
inline constexpr double kExplosionDujZLen = 120.0;

EditBotActionFunc bot_ball_cancelling(const std::u16string& action_id, const Value& desire = Value(),
                                      const Value& keys = Value());

EditBotActionFunc bot_ball_continuation(const std::u16string& action_id, const Value& desire = Value(),
                                        const Value& mp = Value(), const Value& keys = Value());

EditBotActionFunc bot_chasing_action(const std::u16string& action_id, const Value& keys,
                                     const Value& min_mp = Value(), const Value& desire = Value());

EditBotActionFunc bot_chasing_skill_action(const std::u16string& keys_str, const Value& action_id = Value(),
                                           const Value& min_mp = Value(), const Value& desire = Value());

EditBotActionFunc bot_front_test(const std::u16string& action_id, const Value& keys,
                                 const Value& min_mp, const Value& desire = Value(),
                                 const Value& min_x = Value(), const Value& max_x = Value(),
                                 const Value& zable = Value());

EditBotActionFunc bot_ball_dfa(const Value& min_mp, const Value& desire = Value(),
                               const Value& min_x = Value(), const Value& max_x = Value(),
                               const Value& zable = Value());

EditBotActionFunc bot_ball_dfj(const Value& min_mp, const Value& desire = Value(),
                               const Value& min_x = Value(), const Value& max_x = Value(),
                               const Value& zable = Value());

EditBotActionFunc bot_explosion_dua(const Value& min_mp, const Value& desire = Value(),
                                    const Value& min_x = Value(), const Value& max_x = Value(),
                                    const Value& z_len = Value());

EditBotActionFunc bot_explosion_duj(const Value& min_mp, const Value& desire = Value(),
                                    const Value& min_x = Value(), const Value& max_x = Value(),
                                    const Value& z_len = Value());

EditBotActionFunc bot_idle_action(const std::u16string& action_id, const Value& keys,
                                  const Value& min_mp = Value(), const Value& desire = Value());

Value bot_uppercut_dua(const Value& min_mp, const Value& desire = Value(), const Value& min_x = Value(),
                       const Value& max_x = Value(), const Value& max_d = Value());

Value bot_uppercut_duj(const Value& min_mp, const Value& desire = Value(), const Value& min_x = Value(),
                       const Value& max_x = Value());

EditBotActionFunc bot_uppercut_dva(const Value& min_mp, const Value& desire = Value(),
                                   const Value& min_x = Value(), const Value& max_x = Value(),
                                   const Value& max_d = Value());

}
}
}
