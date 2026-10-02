#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bots_types.h"

namespace lfw {
namespace dat_translator {
namespace bots {

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
