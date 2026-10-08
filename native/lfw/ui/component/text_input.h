#pragma once

#include <optional>
#include <string>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/TextInput.ts`（TAGS = ["TextInput"]）：纯文本输入基类（只存 `_text`，
// 不与节点文本联动）。
class TextInput : public UIComponent {
 public:
  static constexpr const char* TAG = "TextInput";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  virtual std::optional<double> max_length();
  virtual std::optional<std::u16string> default_value();

  const std::u16string& text() const { return _text; }
  void set_text(const std::u16string& v) { _text = v; }

 protected:
  std::u16string _text;
};

}
}
