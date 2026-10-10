#pragma once

#include <string>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/BGMSwitcher.ts`（TAGS = ["BGMSwitcher"]）：`args[0]` 是循环偏移；
// 点击在 `Silent / Random / lfw.bgms…` 之间切换（左键 +1 / 中键互切 / 右键 -1），
// `_sounds_cbs` 订 `lfw.sounds.callbacks` 的 `on_bgm_changed` 把结果写成节点文本。
class BGMSwitcher : public UIComponent {
 public:
  static constexpr const char* TAG = "BGMSwitcher";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_resume() override;
  void on_pause() override;
  void on_click(LFWPointerEvent& e) override;

  // TS `get which`：`_which || lfw.sounds.bgm() || "Silent"`。
  std::u16string which();
  // TS `get offset`：`this.num(0) ?? 0`。
  double offset();

 private:
  struct SoundsListener : public LfwSoundsCallback {
    explicit SoundsListener(BGMSwitcher* o) : owner(o) {}
    BGMSwitcher* owner = nullptr;
    void on_bgm_changed(const std::u16string& bgm, const std::u16string& prev) override;
  };

  std::u16string _which;
  SoundsListener _sounds_listener{this};
};

}
}
