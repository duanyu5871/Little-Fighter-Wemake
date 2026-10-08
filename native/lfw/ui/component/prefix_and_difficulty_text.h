#pragma once

#include <functional>
#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/PrefixAndDifficultyText.ts`（TAGS = ["PrefixAndDifficultyText"]）：
// args[0] 是前缀；`world.dataset.difficulty` 变就写 `"<前缀> (<难度名>)"` 到节点文本。
class PrefixAndDifficultyText : public UIComponent {
 public:
  static constexpr const char* TAG = "PrefixAndDifficultyText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_resume() override;
  void on_pause() override;
  void on_dataset_change(const std::u16string& key);

  std::u16string text();

 protected:
  std::u16string _prefix;
  std::function<void()> _remover;
};

}
}
