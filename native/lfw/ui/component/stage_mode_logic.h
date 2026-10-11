#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/lfw.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/game_mode_fsm_state.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {

namespace ui {

class ComponentsPlayer;
class FighterStatBar;
class Jalousie;

// TS `ui/component/StageModeLogic.ts`（TAGS = ["StageModeLogic"]）：闯关模式页
// （FSM 驱动、按玩家数启用/挂接血条、换关 gogogo/门帘、结束态按键弹页）。
//
// `world_callbacks` / `stage_callbacks` 两个回调对象端口化成「remover 闭包」对：
// `listen_stage` 换舞台时先撤旧再订新（对应 TS `prev.callbacks.del` + `stage.callbacks.add`）。
class StageModeLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "StageModeLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  StageModeLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  FSM* fsm() override { return &_fsm; }

  void handle_stage_actions(const Value& actions);
  void on_start() override;
  void on_stop() override;
  void update(double dt) override;
  void on_key_down(LFWKeyEvent& e) override;

  Jalousie* jalousie() const { return _jalousie; }
  ComponentsPlayer* gogogo() const { return _gogogo; }
  ComponentsPlayer* gogogo_loop() const { return _gogogo_loop; }
  UINode* rank_hint_cheat_node() const { return _rank_hint_cheat; }
  UINode* rank_hint_mods_node() const { return _rank_hint_mods; }

  FSM _fsm;

 private:
  void listen_stage(stage::Stage* stage);
  void handle_phase_changed(const stage::StageCallbackArgs& a);
  void remove_listeners();

  std::vector<std::unique_ptr<GameModeFSMState>> _states;
  Jalousie* _jalousie = nullptr;
  ComponentsPlayer* _gogogo = nullptr;
  ComponentsPlayer* _gogogo_loop = nullptr;
  UINode* _rank_hint_cheat = nullptr;
  UINode* _rank_hint_mods = nullptr;
  std::function<void()> _stage_remover_pc;
  std::function<void()> _stage_remover_req;
  std::function<void()> _world_remover;
};

}
}
