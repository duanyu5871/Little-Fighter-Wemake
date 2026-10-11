#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/lfw.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/game_mode_fsm_state.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {

class Entity;

namespace ui {

class CameraCtrl;
class ComponentsPlayer;
class Jalousie;

// TS `ui/component/DemoModeLogic.ts`（TAGS = ["DemoModeLogic"]）：演示模式页
// （抽签出一场演出：模式/背景/角色全随机，镜头自动跟随，结束态门帘翻页）。
//
// TS 的四个静态字段（`_situations` / `_situation` / `_stages`）端口化成 .cpp 里的
// 文件级单例（绑首个调用的 LFW 的 `mt`，与 TS 静态成员同一行为）。
class DemoModeLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "DemoModeLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  DemoModeLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const Value& props_meta() const override { return PROPS(); }
  FSM* fsm() override { return &_fsm; }

  UINode* focus_prefix();
  UINode* focus_on();
  CameraCtrl* cam_ctrl();
  UINode* score_board();
  UINode* situation_name();
  UINode* focus_text_node();
  Jalousie* jalousie();
  ComponentsPlayer* gogogo();
  ComponentsPlayer* gogogo_loop();

  bool is_stage_mode();
  bool is_vs_mode();

  void handle_stage_actions(const Value& actions);
  void startup();
  void clearup();

  void on_start() override;
  void on_stop() override;
  void update(double dt) override;
  void on_key_down(LFWKeyEvent& e) override;

  FSM _fsm;

 private:
  static Randoming& get_situations(LFW& lfw);
  static const Value& get_situation(LFW& lfw);
  static void clear_situation();
  static Randoming& get_stages(LFW& lfw);

  void listen_stage(stage::Stage* stage);
  void handle_phase_changed(const stage::StageCallbackArgs& a);
  void remove_listeners();

  std::vector<std::unique_ptr<GameModeFSMState>> _states;
  std::optional<Entity*> _staring;
  std::optional<bool> _free;
  stage::Stage* _stage = nullptr;
  std::function<void()> _stage_remover_pc;
  std::function<void()> _stage_remover_req;
  std::function<void()> _world_remover;
};

}
}
