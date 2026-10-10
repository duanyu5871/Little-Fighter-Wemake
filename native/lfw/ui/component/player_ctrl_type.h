#pragma once

#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {

class PlayerInfo;

namespace ui {

// TS `ui/component/PlayerCtrlType.ts`：`player_id` 取 args[0] 或祖先 `values.player_id`；
// 点击按 `(ctrl ± 1) % 6` 轮换（右键是 +5 = -1），换到触屏时把其它触屏玩家压回键盘。
class PlayerCtrlType : public UIComponent {
 public:
  static constexpr const char* TAG = "PlayerCtrlType";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::u16string player_id();
  PlayerInfo* player();

  void on_resume() override;
  void on_pause() override;
  void on_click(LFWPointerEvent& e) override;
  void on_ctrl_changed(const Value& c);

 private:
  Callbacks::Remover _remover;
};

}
}
