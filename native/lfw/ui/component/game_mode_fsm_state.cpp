#include "lfw/ui/component/game_mode_fsm_state.h"

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

FSM* GameModeFSMState::fsm() { return owner.fsm(); }

std::optional<Value> GameModeFSMState_Running::update(double dt) {
  (void)dt;
  if (world.paused()) return std::nullopt;
  if (!world.game_result(false).empty()) return Value(mode_state_str(ModeState::BeforeEnd));
  return std::nullopt;
}

void GameModeFSMState_BeforeEnd::enter() { _time = 0.0; }

void GameModeFSMState_BeforeEnd::leave() { _time = 0.0; }

std::optional<Value> GameModeFSMState_BeforeEnd::update(double dt) {
  if (world.paused()) return std::nullopt;
  _time += dt;
  if (_time < 3000.0) return std::nullopt;
  return Value(mode_state_str(
      world.game_result(false).empty() ? ModeState::Running : ModeState::End));
}

void GameModeFSMState_End::enter() {
  if (world.game_result(false) == u"win") {
    lfw.host().sounds_play_preset(Value(u"pass"), Value(), Value(), Value());
  } else {
    lfw.host().sounds_play_preset(Value(u"end"), Value(), Value(), Value());
  }
  UINode* const score_board = node.find_child(u"score_board");
  if (score_board != nullptr) score_board->set_visible(true);
}

void GameModeFSMState_End::leave() {
  UINode* const score_board = node.find_child(u"score_board");
  if (score_board != nullptr) score_board->set_visible(false);
}

}
}
