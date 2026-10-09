#pragma once

#include <set>
#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class IntegerPicker;

// TS `IIntegerPickerCallbacks`（`on_min_changed` / `on_max_changed` / `on_val_changed`，
// 参数是 `(curr, prev, self)`）——端口用带类型的总线 payload 顶替 `self` 指针。
struct IntegerPickerCallbackArgs {
  double curr = 0.0;
  double prev = 0.0;
  IntegerPicker* self = nullptr;
};
using IntegerPickerCallbacks = CallbacksT<IntegerPickerCallbackArgs>;

// TS `ui/component/IntegerPicker.ts`（TAGS = ["IntegerPicker"]）：props = `{ min, max, val, triggers }`；
// 点击（0 下一档 / 2 上一档）或按键（aj/ud/lr 三组开关 + `no-focused`）改值，值写回节点文本。
class IntegerPicker : public UIComponent {
 public:
  static constexpr const char* TAG = "IntegerPicker";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  IntegerPickerCallbacks callbacks;

  void init() override;
  void on_click(LFWPointerEvent& e) override;
  void on_key_down(LFWKeyEvent& e) override;

  double val() const { return _val; }
  double min() const { return _min; }
  double max() const { return _max; }

  void goto_next();
  void goto_prev();
  void set_min(double v);
  void set_max(double v);
  void set_val(double v);

 protected:
  // `Number.MIN_SAFE_INTEGER` / `MAX_SAFE_INTEGER`。
  double _min = -9007199254740991.0;
  double _max = 9007199254740991.0;
  double _val = _min;
  std::set<std::u16string> _triggers;
};

}
}
