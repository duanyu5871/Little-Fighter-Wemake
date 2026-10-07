#pragma once

#include <string>

#include "lfw/base/expression.h"
#include "lfw/bot/bot_controller.h"

namespace lfw {
namespace loader {

// `loader/get_val_from_bot_ctrl.ts`：bot 表达式（`IBotAction.expression`）的「词 → getter」
// 表。命中 `bot_val_getters`（`defines/BotVal`）的九个词走 bot 自己的读法；其余回落到
// 「实体 getter 表」（`get_val_getter_from_entity`，经 `CtrlEnv::entity_val` 缝），
// 再兜不住就给 `word` 本身（TS 的 `() => word`）。
ValGetter<bot::BotController> get_val_from_bot_ctrl(const std::u16string& word);

}  // namespace loader
}  // namespace lfw
