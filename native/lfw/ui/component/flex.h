#pragma once

#include <array>
#include <string>
#include <vector>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/Flex.ts`（TAGS = ["Flex"]）：行列自适应布局。
//   * props `direction`（row/column）/`gap`/`row_gap`/`col_gap`/`align`/`fit`/`fit_w`/`fit_h`/`padding`
//     —— `row_gap`/`col_gap` 缺省落到 `gap`，`fit_w`/`fit_h` 缺省落到 `fit`。
//   * `apply()` 两段：先按 `fit_*` 算自身尺寸（**跳隐藏子**，但 gap 用**原始下标**判尾），
//     再对可见子节点逐个摆位/拉伸（对齐值可取 `FlexItem.align` 覆盖）。
//   * TS 是 `Flex<Callbacks>` 泛型类（给 Jalousie 之类传回调类型）⇒ 端口不需要泛型。
class Flex : public UIComponent {
 public:
  static constexpr const char* TAG = "Flex";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::u16string direction();
  double gap();
  double row_gap();
  double col_gap();
  std::u16string align();
  bool fit();
  bool fit_w();
  bool fit_h();
  std::array<double, 4> padding();

  void on_resume() override;
  void update(double dt) override;

  void apply();
};

}
}
