#pragma once

#include <string>

#include "lfw/animation/sine.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/FighterName.ts`（TAGS = ["FighterName"]）：角色名文本；节点自带 style 就用它，
// 否则内联（`com` 决定粉色）。未决定时 `Sine(0.65, 1, 6)` 闪烁。
class FighterName : public UIComponent {
 public:
  static constexpr const char* TAG = "FighterName";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void join(const std::u16string& text, bool com, bool decided);
  void quit();
  void update(double dt) override;

  Value make_style(bool com);

 protected:
  bool _decided = false;
  bool _com = false;
  Sine _opacity{0.65, 1, 6};
};

}
}
