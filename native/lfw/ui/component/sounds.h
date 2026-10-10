#pragma once

#include <string>
#include <vector>

#include "lfw/animation/loop.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/Sounds.ts`（`IPlayable`：start/stop/replay）。
// args = [间隔, 声音名, 间隔, 声音名, …]；名字命中 `Defines.BuiltIn_Sounds` 的**键名**
// （`Cancel`/`End`/`Join`/`Ok`/`Pass`，注意不是小写值）走 `play_with_load`，否则 `play_preset`。
class Sounds : public UIComponent {
 public:
  static constexpr const char* TAG = "Sounds";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  struct Step {
    double t = 0.0;
    std::u16string s;
    bool builtin = false;
  };

  std::vector<Step> seq;
  double idx = 0.0;
  double time = 0.0;
  Loop loop;

  // TS 返回 `this`（链式）；端口的 `UIComponent::start/stop` 是 `void` ⇒ 只能改签名（记偏差）。
  void start() override;
  void stop() override;
  void replay() override;

  bool is_playable() const override { return true; }

  void on_start() override;
  void on_resume() override;
  void update(double dt) override;
};

}
}
