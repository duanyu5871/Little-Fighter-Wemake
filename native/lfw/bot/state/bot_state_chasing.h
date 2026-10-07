#pragma once

#include <optional>

#include "lfw/bot/state/bot_state.h"

namespace lfw {
namespace bot {

// `bot/state/BotState_Chasing.ts`
class BotState_Chasing : public BotState_Base {
 public:
  explicit BotState_Chasing(BotController& ctrl);
  std::optional<Value> update(double dt) override;

 private:
  std::optional<Value> update_dash();
  std::optional<Value> update_running();
  std::optional<Value> update_jump();
};

}  // namespace bot
}  // namespace lfw
