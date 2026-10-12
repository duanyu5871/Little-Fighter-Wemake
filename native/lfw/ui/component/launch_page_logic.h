#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class ImgLoop;
class UINode;

// TS `ui/component/LaunchPageLogic.ts`（`TAGS = ["LaunchPage"]`）：启动页四态 FSM
// （TapHints → Introduction → GoToEntry → End）。
class LaunchPageLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "LaunchPage";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  LaunchPageLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  FSM* fsm() override { return &_fsm; }

  void on_start() override;
  void on_stop() override;
  void on_resume() override;
  void on_key_down(LFWKeyEvent& e) override;
  void on_click(LFWPointerEvent& e) override;
  void update(double dt) override;

  void on_tap();

  bool prel_loaded() const { return _prel_loaded; }
  ImgLoop* loading_anim();
  UINode* translation_note();
  // TS `on_resume` 里对 props 的赋值实际是空操作（setter 为空），取值始终走惰性解析。
  UINode* tap_to_launch();
  UINode* sound_warning();
  UINode* yeonface();
  UINode* bearface();
  UINode* long_text();

 private:
  void on_prel_loaded();

  struct TapHintsState;
  struct IntroductionState;
  struct GoToEntryState;
  struct EndState;

  FSM _fsm;
  bool _prel_loaded = false;
  std::vector<std::unique_ptr<IState>> _states;
  std::function<void()> _cb_remover;
};

}
}
