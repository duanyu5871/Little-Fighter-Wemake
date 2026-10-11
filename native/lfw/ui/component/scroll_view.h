#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class ScrollView;
class SliderHandle;

// TS `IScrollViewCallbacks`（`on_scroll(offset, factor, component)` / `on_scroll_start(component)`
// / `on_scroll_end(component)`）——端口用带类型的总线 payload 顶替对象参数。
struct ScrollViewCallbackArgs {
  double offset = 0.0;
  double factor = 0.0;
  ScrollView* self = nullptr;
};
using ScrollViewCallbacks = CallbacksT<ScrollViewCallbackArgs>;

// TS `ui/component/ScrollView.ts`（TAGS = ["ScrollView"]）：滚动视图。
// 视口（本组件所在节点）启用 `clip_children`；内容节点（`content`）整体平移；
// 支持指针拖拽 / 方向键 / 滚轮 / 焦点跟随 / 可选裁剪（`cull`）与滚动条（SliderHandle 或
// 直接受控的 thumb 节点）双向同步。
class ScrollView : public UIComponent {
 public:
  static constexpr const char* TAG = "ScrollView";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  ScrollViewCallbacks callbacks;

  void on_start() override;
  void on_stop() override;
  void on_resume() override;
  void update(double dt) override;

  std::u16string direction();
  bool is_col();
  double step();
  double speed();
  bool draggable();
  bool keyboard();
  bool wheel_enabled();
  double wheel_speed();
  bool cull();
  double drag_threshold();
  bool follow_focus();

  // 内容节点（被滚动的节点）；缺省 = 视口第一个可见子节点（没有则第一个子节点）。
  UINode* content();
  void set_content(UINode* v);
  // 直接受控的滚动条 thumb 节点（跨子树按 id 兜底查找并缓存）。
  UINode* scrollbar_thumb();
  // 关联的滚动条 SliderHandle（跨子树按组件 id 兜底查找并缓存；找不到不缓存）。
  SliderHandle* scrollbar();

  // 可滚动像素（内容超出视口的部分）；内容不足一屏时为 0。
  double max_scroll();
  bool dragging();
  double offset();
  // 目标滚动像素（平滑的收敛点；台面探针用）。
  double target_offset() { return _target_offset; }
  void set_offset(double v);
  double factor();
  void set_factor(double v);

  ScrollView& scroll_to(double offset);
  ScrollView& scroll_by(double delta);
  ScrollView& scroll_to_start();
  ScrollView& scroll_to_end();
  void on_scroll(std::function<void(double, double, ScrollView&)> cb);
  // 滚动使指定节点可见（节点需位于内容子树内）。返回值照抄 TS 的比较口径。
  bool ensure_visible(UINode* node);

 protected:
  double key_dir();
  void update_keys(double dt);
  UINode* focused_in(UINode* c);
  void unfocus_content();
  double pointer_axis_px(const LfwPointingEvent& e);
  // TS 里的死代码（无引用），端口保留结构。
  bool pointer_in_view(const LfwPointingEvent& e);
  bool pointer_in_node(const LfwPointingEvent& e, UINode* n);
  void scrollbar_pointer(const LfwPointingEvent& e);
  void end_drag();
  void sync_scrollbar();
  void sync_thumb(UINode* thumb);

  // 可滚动区间（内容起点/终点允许的平移量，相对基础位置，单位 px）。
  std::pair<double, double> range();
  double window_lo();
  std::pair<double, double> box(UINode* c);
  std::pair<double, double> node_range(UINode* n);
  void apply_offset();
  void update_cull();
  bool is_culled(UINode* n) const;

  // TS 的 `p`（`IPointingsCallback`）——拖拽/滚轮，注册进宿主回调表。
  struct PointListener : public LfwPointingsCallback {
    explicit PointListener(ScrollView* o) : owner(o) {}
    ScrollView* owner = nullptr;
    void on_pointer_down(LfwPointingEvent& e) override;
    void on_pointer_move(LfwPointingEvent& e) override;
    void on_pointer_up(LfwPointingEvent& e) override;
    void on_pointer_cancel(LfwPointingEvent& e) override;
    void on_wheel(LfwPointingEvent& e) override;
  };

  UINode* _content = nullptr;
  bool _captured = false;
  double _base_x = 0;
  double _base_y = 0;

  // 当前实际滚动的像素（0 ~ max_scroll，从"内容起点对齐视口起点"算起）。
  double _offset = 0;
  // 目标滚动像素（带平滑）。
  double _target_offset = 0;
  double _reported = -1;

  // 被本组件隐藏（裁剪）的内容子节点。
  std::vector<UINode*> _culled;
  std::map<UINode*, bool> _cull_orig;

  // 拖拽。
  bool _armed = false;
  bool _dragging = false;
  double _last_axis = 0;
  double _drag_dist = 0;

  // 键盘。
  bool _key_active = false;
  double _key_time = 0;

  // 滚动条同步。
  std::optional<double> _last_slider_factor;
  SliderHandle* _scrollbar = nullptr;
  UINode* _sb_thumb = nullptr;
  bool _sb_dragging = false;
  // 拖动时指针相对 thumb 起始端的偏移（点中轨道时=len/2，使 thumb 居中于指针）。
  double _sb_grab = 0;

  PointListener _p{this};
};

}
}
