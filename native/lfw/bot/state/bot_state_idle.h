#pragma once

#include <optional>

#include "lfw/bot/state/bot_state.h"

namespace lfw {
namespace bot {

// `bot/state/BotState_Idle.ts`
class BotState_Idle : public BotState_Base {
 public:
  explicit BotState_Idle(BotController& ctrl);
  void enter() override;
  void leave() override;
  std::optional<Value> update(double dt) override;
};

}  // namespace bot
}  // namespace lfw
