#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bots_types.h"

namespace lfw {
namespace dat_translator {
namespace bots {

EditBotActionFunc bot_ball_cancelling(const std::u16string& action_id, const Value& desire,
                                      const Value& keys);

EditBotActionFunc bot_ball_continuation(const std::u16string& action_id, const Value& desire,
                                        const Value& mp, const Value& keys);

EditBotActionFunc bot_chasing_action(const std::u16string& action_id, const Value& keys,
                                     const Value& min_mp, const Value& desire);

EditBotActionFunc bot_chasing_skill_action(const std::u16string& keys_str, const Value& action_id,
                                           const Value& min_mp, const Value& desire);

EditBotActionFunc bot_front_test(const std::u16string& action_id, const Value& keys,
                                 const Value& min_mp, const Value& desire, const Value& min_x,
                                 const Value& max_x, const Value& zable);

EditBotActionFunc bot_ball_dfa(const Value& min_mp, const Value& desire, const Value& min_x,
                               const Value& max_x, const Value& zable);

EditBotActionFunc bot_ball_dfj(const Value& min_mp, const Value& desire, const Value& min_x,
                               const Value& max_x, const Value& zable);

EditBotActionFunc bot_explosion_dua(const Value& min_mp, const Value& desire, const Value& min_x,
                                    const Value& max_x, const Value& z_len);

EditBotActionFunc bot_explosion_duj(const Value& min_mp, const Value& desire, const Value& min_x,
                                    const Value& max_x, const Value& z_len);

EditBotActionFunc bot_idle_action(const std::u16string& action_id, const Value& keys,
                                  const Value& min_mp, const Value& desire);

Value bot_uppercut_dua(const Value& min_mp, const Value& desire, const Value& min_x,
                       const Value& max_x, const Value& max_d);

Value bot_uppercut_duj(const Value& min_mp, const Value& desire, const Value& min_x,
                       const Value& max_x);

EditBotActionFunc bot_uppercut_dva(const Value& min_mp, const Value& desire, const Value& min_x,
                                   const Value& max_x, const Value& max_d);

}
}
}
