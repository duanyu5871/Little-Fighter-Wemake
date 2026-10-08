#pragma once

#include <functional>
#include <string>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/FnKeysCounts.ts`：props 是 7 个 UINode 引用；
// 接 `world.callbacks` 的 `on_fn_locked_change` / `on_counts` 切显隐与写计数。
class FnKeysCounts : public UIComponent {
 public:
  static constexpr const char* TAG = "FnKeysCounts";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  void on_start() override;
  void on_stop() override;

  void on_fn_locked_change(bool locked);
  void on_counts();

 protected:
  // `props.xxx?.`：取 UINode 引用值（未给 / 解析不到 ⇒ nullptr）。
  UINode* node_prop(const std::u16string& name);

  std::function<void()> _remove_locked;
  std::function<void()> _remove_counts;
};

}
}
