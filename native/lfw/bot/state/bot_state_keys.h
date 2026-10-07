#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/bot/state/bot_state.h"
#include "lfw/defines/bot_state_enum.h"
#include "lfw/defines/game_key.h"

namespace lfw {
namespace bot {

// `BSE`（`defines/BotStateEnum.ts`）。
namespace bse = ::lfw::bot_state_enum;

// `key_up(...AGK)`：`defines/GameKey.ts` 的全键表。
inline std::vector<std::u16string> all_game_key_names() {
  std::vector<std::u16string> out;
  for (const char16_t* k : all_game_keys()) out.emplace_back(k);
  return out;
}

}  // namespace bot
}  // namespace lfw
