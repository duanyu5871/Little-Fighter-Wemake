#pragma once

#include <string>

#include "lfw/animation/sine.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayerTeamName.ts`（TAGS = ["PlayerTeamName"]）：队伍名文本，
// 样式内联（`com` 决定粉色），未决定时 `Sine(0.65, 1, 3)` 闪烁。
class PlayerTeamName : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayerTeamName";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void join(const std::u16string& text, bool com, bool decided);
  void quit();
  void update(double dt) override;

 protected:
  bool _decided = false;
  bool _com = false;
  Sine _opacity{0.65, 1, 3};
};

}
}
