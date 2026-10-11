#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class SliderHandle;

// TS `ISliderHandleCallbacks`（`on_value_changed(value, component)`）——端口用带类型的总线
// payload 顶替第二个参数（`Value` 装不下对象）。
struct SliderHandleCallbackArgs {
  double value = 0.0;
  SliderHandle* self = nullptr;
};
using SliderHandleCallbacks = CallbacksT<SliderHandleCallbackArgs>;

// TS `ui/component/Slider/SliderHandle.ts`（TAGS = ["SliderHandle"]）：滑块手柄。
// 轨迹容器（`container`）负责尺寸/几何，手柄（本组件所在节点）沿 row/col 方向移动；
// `factor`（0..1）是内部真值，`value` = `min + (max-min)*factor` 圆整到 `precision`。
// 三种模式：`default`（普通拖动） / `switcher`（按 items 数等分，手柄盖住当前格） /
// `items_scroll_bar`（按 items_container 子节点数占比决定手柄长度）。
// 交互：点轨迹容器（node 回调）聚焦并抢焦点（grab）；拖动/松开走平台回调表（`b`）。
class SliderHandle : public UIComponent {
 public:
  static constexpr const char* TAG = "SliderHandle";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  SliderHandleCallbacks callbacks;

  void on_start() override;
  void on_stop() override;
  void update(double dt) override;

  std::u16string direction();
  virtual std::u16string mode();
  void set_mode(const std::u16string& v);
  std::vector<std::u16string> items();
  void set_items(const std::vector<std::u16string>& v);
  // `props.container ?? node.parent` / `props.responser ?? container`（节点型属性用
  // `prop_instance` + `ref_to_node`，与 Dialogs 同口径）。
  UINode* container();
  UINode* responser();
  UINode* handle_label();
  UINode* items_container();
  double step();
  void set_step(double v);
  double precision();
  void set_precision(double v);
  double min_value();
  void set_min_value(double v);
  double max_value();
  void set_max_value(double v);
  // `get factor`（钳到 0..1）/ `set factor`（`set_factor`）。
  double factor();
  void set_factor(double v);
  // `get value`（按 `_factor` 换算）/ `set value`（`set_value`）。
  double value();
  void set_value(double v);
  // TS `on_value_changed(cb)`：注册进 `callbacks` 总线。
  void on_value_changed(std::function<void(double, SliderHandle&)> cb);
  // 指针事件 → factor/value（row 用 scene_x、col 用 scene_y）。
  void handle_pointing_event(const LfwPointingEvent& e);

 protected:
  // 点轨迹：聚焦 responser/container + grab；0..1 整数开关直接切换，否则进入拖动（`_on_me`）。
  void on_p_down(LFWPointerEvent& e);

  // TS 的 `b`（`IPointingsCallback`）——拖动/松开/取消，注册进宿主回调表。
  struct PointListener : public LfwPointingsCallback {
    explicit PointListener(SliderHandle* o) : owner(o) {}
    SliderHandle* owner = nullptr;
    void on_pointer_down(LfwPointingEvent& e) override;
    void on_pointer_move(LfwPointingEvent& e) override;
    void on_pointer_up(LfwPointingEvent& e) override;
    void on_pointer_cancel(LfwPointingEvent& e) override;
  };

  bool _on_me = false;
  double _factor = 0.0;
  PointListener _b{this};
};

}
}
