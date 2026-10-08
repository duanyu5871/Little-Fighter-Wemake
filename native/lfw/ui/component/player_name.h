#pragma once

#include <string>

#include "lfw/animation/sine.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayerName.ts`（TAGS = ["PlayerName"]）：props = `{ prefix, hide_join_q }`。
// `join` 写「前缀 + 名字」，`quit` 写「加 入?」（`hide_join_q` 为真时顺手隐藏），
// 未决定时透明度走 `Sine(0.65, 0.35, 3)` 闪烁。
class PlayerName : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayerName";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  void join(const std::u16string& text, bool com, bool decided);
  void quit();
  void update(double dt) override;

  Value make_style(bool com);

 protected:
  bool _decided = false;
  bool _com = false;
  Sine _opacity{0.65, 0.35, 3};
};

}
}
