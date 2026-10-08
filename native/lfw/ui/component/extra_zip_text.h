#pragma once

#include <functional>
#include <string>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/ExtraZipText.ts`：接 `lfw.callbacks` 的 `on_extra_zips_changed` / `on_lang_changed`，
// 把 `DATA_LIST` 词条写进节点文本（空则写一个空格）。
class ExtraZipText : public UIComponent {
 public:
  static constexpr const char* TAG = "ExtraZipText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;

  void render();

 protected:
  std::function<void()> _remove_zips;
  std::function<void()> _remove_lang;
};

}
}
