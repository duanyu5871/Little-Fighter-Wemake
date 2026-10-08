#pragma once

#include <string>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayingTimeText.ts`：`on_show` 时把 `world.stage.time` 写成 mm:ss。
class PlayingTimeText : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayingTimeText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_show() override;
  std::u16string get_txt();
};

}
}
