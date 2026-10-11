#pragma once

#include <optional>
#include <string>

#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/style.h"

namespace lfw {
namespace ui {

class PlayerScore;

// TS `ui/component/PlayerScoreCell.ts`（TAGS = ["PlayerScoreCell"]）：`args[0]` 是
// 单元格类型（kill / attack / hp_lost / mp_usage / picking / status），内容走
// `summary_mgr`，样式按胜负状态改 `node.style.fill_style`；带 (文本, 样式版本) 缓存。
class PlayerScoreCell : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayerScoreCell";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::u16string kind();
  PlayerScore* player_score();

  void update(double dt) override;

 protected:
  std::optional<std::u16string> _last_txt;
  double _last_style_version = -1;

  Style& get_style();
  std::u16string get_txt();
};

}
}
