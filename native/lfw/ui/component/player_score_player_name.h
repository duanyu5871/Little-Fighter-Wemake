#pragma once

#include <string>
#include <vector>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayerScorePlayerName.ts`（TAGS = ["PlayerScorePlayerName"]）：
// `on_show` 时按队伍颜色写名字（描边 + 12px Arial）。
class PlayerScorePlayerName : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayerScorePlayerName";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_show() override;
};

}
}
