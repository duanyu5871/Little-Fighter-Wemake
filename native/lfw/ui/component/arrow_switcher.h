#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class ArrowSwitcher;

// TS `IArrowSwitcherCallbacks`（`on_value_changed(value, component)`）。
struct ArrowSwitcherCallbackArgs {
  double value = 0.0;
  ArrowSwitcher* self = nullptr;
};
using ArrowSwitcherCallbacks = CallbacksT<ArrowSwitcherCallbackArgs>;

// TS `ui/component/Slider/ArrowSwitcher.ts`（TAGS = ["ArrowSwitcher"]）：左右箭头切换器。
// 只显示当前项（`label` 节点），点箭头或按 ◀/▶ 循环切换（wrap）；
// 高亮叠层（left_hl/right_hl）按箭头 hover 平滑淡入淡出。
class ArrowSwitcher : public UIComponent {
 public:
  static constexpr const char* TAG = "ArrowSwitcher";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  ArrowSwitcherCallbacks callbacks;

  void on_start() override;
  void on_resume() override;
  void on_stop() override;
  void update(double dt) override;

  std::vector<std::u16string> items();
  double count();
  // `get value`：`_value` 越界自动钳到 `count-1`。
  double value();
  UINode* left();
  UINode* right();
  UINode* left_hl();
  UINode* right_hl();
  UINode* label();
  // `props.responser ?? node`（缺省节点型属性 ⇒ 本组件所在节点）。
  UINode* responser();
  void on_value_changed(std::function<void(double, ArrowSwitcher&)> cb);
  // 点箭头：聚焦 responser 后切换。
  void press(double dir);
  // 切换（循环 wrap）并回调。
  void step(double dir);
  // 直接设置下标（钳制，不回调）。
  void set_value(double v);

 protected:
  // hover 高亮平滑（约 90ms）。
  void update_highlights(double dt);
  void sync_label();
  void sync_arrows();

  double _value = 0.0;
  double _hl_l = 0.0;
  double _hl_r = 0.0;
};

}
}
