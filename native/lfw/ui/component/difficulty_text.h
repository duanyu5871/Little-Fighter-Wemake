#pragma once

#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/DifficultyText.ts`（TAGS = ["DifficultyText", "difficulty_text"]）：
// `world.callbacks` 的 `on_dataset_change` → 把 `dataset.difficulty` 的标签写上节点文本。
class DifficultyText : public UIComponent {
 public:
  static constexpr const char* TAG = "DifficultyText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_resume() override;
  void on_pause() override;
  void on_dataset_change(const std::u16string& key);

 protected:
  std::function<void()> _remover;
};

}
}
