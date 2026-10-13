#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/lfw.h"

namespace lfw {

namespace ui {

class UINode;
class UILayer;

// TS `ui/UILayer.ts` 的 `IUILayersCallback`（`Callbacks<IUILayersCallback>` 端口化成可选字段表）。
struct UILayersCallbacks {
  // `on_set(pushed, popped, layer)`
  std::function<void(UINode*, UINode*, UILayer&)> on_set;
  // `on_push(pushed, prev, layer)`
  std::function<void(UINode*, UINode*, UILayer&)> on_push;
  // `on_pop(curr, poppeds, layer)`
  std::function<void(UINode*, const std::vector<UINode*>&, UILayer&)> on_pop;
};

// TS `IPopPageOpts`。
struct UIPopPageOpts {
  bool inclusive = false;
  // `until(ui, index, pages)`
  std::function<bool(UINode&, double, const std::vector<UINode*>&)> until;
  double min_pages = 0.0;
};

// TS `IPushPageOpts`（`id` 只接受字符串，同 TS 的 `is_str` 判定）。
struct UIPushPageOpts {
  std::u16string id;
  bool has_id = false;
};

// TS `ui/UILayer.ts` 的 `UILayer`。页面由本层持有（TS 靠 GC，端口用 `unique_ptr`）。
class UILayer {
 public:
  UILayer(LFW& lfw, double index);
  // 切片 4CT：定义在 .cpp —— `_pages` / `_trash` 里的 `unique_ptr<UINode>` 需要完整类型才能析构
  // （本头只前置声明 `UINode`）。
  ~UILayer();

  LFW& lfw() const { return *_lfw; }
  double index() const { return _index; }
  std::vector<UINode*> pages() const;
  UINode* ui() const;
  UINode* at(double idx) const;

  UILayersCallbacks callbacks;

  void dispose();
  void set(const UIPushPageOpts& opts);
  void push(const UIPushPageOpts& opts);
  void pop(const UIPopPageOpts& opts);

 private:
  std::unique_ptr<UINode> create_page(const UIPushPageOpts& opts);
  LFW* _lfw = nullptr;
  double _index = 0;
  std::vector<std::unique_ptr<UINode>> _pages;
  // 被弹/被替换页的延迟销毁池：`pop`/`set` 里把旧页移到这里，下一次层级变更操作才真正
  // 销毁。TS 里页对象只靠 GC 存活，弹出后调用方（如 `CharMenuLogic::press_j` 在 pop 后
  // 继续播声/刷槽位）仍在使用；端口立即销毁会在同一次调用里触发 UAF。
  std::vector<std::unique_ptr<UINode>> _trash;
};

// TS `ui/UILayer.ts` 的 `UILayers`（切片 4CT / ck2：`IUiLayers` 缝退役 ⇒ 本类就是
// `LFW::layers` 的真类型，`push/set_page/push_page/dispose/ui` 六面留在本类上）。
//
// 偏差：TS 的非整数/负数层号会挂在数组对象属性上（不进 `all`、不计长度）；端口把这些层
// 放进 `_loose`（同样不进 `all`），但 `at()` 只认非负整数下标（与 JS 数组下标一致）。
class UILayers {
 public:
  explicit UILayers(LFW& lfw);

  LFW& lfw() const { return *_lfw; }
  UILayer* bottom() const;
  UILayer* top() const;
  // `Array.from(this._all)`：洞（未 ensure 的下标）保留为 nullptr。
  std::vector<UILayer*> all() const;
  UINode* ui_top() const;
  double length() const;
  UILayer& push_layer();
  UILayer& ensure(double index);
  UILayer* at(double index);

  // 原 `IUiLayers` 的六面（4CT 起就是本类自己的方法）。
  void push();
  void set_page(const Value& opts, double index);
  void push_page(const Value& opts, double index);
  void dispose();
  UINode* ui();
  // `LFW::layer_nodes()` 的面：各层的 `ui`（洞与无页的层跳过）。
  std::vector<UINode*> layer_nodes();

 private:
  static UIPushPageOpts to_opts(const Value& opts);
  LFW* _lfw = nullptr;
  std::vector<std::unique_ptr<UILayer>> _all;
  std::vector<std::unique_ptr<UILayer>> _loose;
};

}  // namespace ui
}  // namespace lfw
