#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/ui/component/slider_handle.h"

namespace lfw {
namespace ui {

// TS `ui/component/Slider/SwitcherHandle.ts`（TAGS = ["SwitcherHandle"]）：分段切换手柄。
// 继承 SliderHandle，缺省即 `switcher` 模式；额外负责 `options` 容器内前 N 个子节点
// （选项 label）的均分布局与「被手柄盖住那格隐藏」。
class SwitcherHandle : public SliderHandle {
 public:
  static constexpr const char* TAG = "SwitcherHandle";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using SliderHandle::SliderHandle;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  void on_start() override;
  void on_resume() override;
  void update(double dt) override;

  // `get mode` 缺省 `'switcher'`（setter 与基类相同）。
  std::u16string mode() override;

  UINode* options();
  // `props.slots ?? options?.children.length ?? 0`。
  double slots();

 protected:
  // 让 min/max/precision/step 与 items 数量对齐（items 变化时自动收敛）。
  void sync_range();
  // 当前被手柄盖住的单元格下标（-1 = 算不出）。
  double cover_index();
  // 选项 label 均分布局；`cover` 缺省按手柄位置推算。
  void arrange_options(std::optional<double> cover = {});
};

}
}
