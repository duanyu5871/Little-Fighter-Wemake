#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/bot/bot_env.h"
#include "lfw/core/value.h"

namespace lfw {
namespace bot {

class BotController;

// `BSE.X` 的下个状态（`update()` 的返回值）。
inline std::optional<Value> next_state(const char16_t* key) {
  return std::optional<Value>(Value(std::u16string(key)));
}

// `bot/state/BotState.ts` 的 `BotState_Base implements IState<BotStateEnum>`。
// 子类在构造里把 `key`（`BSE.*`）填进 `key_`；所有取值都走 `ctrl_` 的 `CtrlEnv` / 引用
// （端口里 bot 层看不见 `Entity`，见 `bot_env.h`）。
class BotState_Base : public IState {
 public:
  explicit BotState_Base(BotController& ctrl) : ctrl_(ctrl) {}

  Value key() const override { return key_; }

  BotController& ctrl() const { return ctrl_; }

  // `this.difficulty` / `this.en` / `this.av`。
  double difficulty() const;
  Value en() const;
  Value av() const;
  // `closest(this.me, ...list)`
  Value closest(const std::vector<Value>& list) const;

  // `wanted_jumping(where)` / `random_jumping(where)`
  bool wanted_jumping(const std::u16string& where);
  bool random_jumping(const std::u16string& where);
  // `handle_defends(mark)` / `handle_block()` / `handle_bot_actions(where)`
  bool handle_defends(const std::u16string& mark);
  bool handle_block();
  bool handle_bot_actions(const std::u16string& where);
  // `hold_UD(rz, min_z, max_z)` / `hold_LR(rx, min_x, max_x)`
  void hold_UD(double rz, double min_z, double max_z);
  void hold_LR(double rx, double min_x, double max_x);

 protected:
  BotController& ctrl_;
  Value key_;
};

}  // namespace bot
}  // namespace lfw
