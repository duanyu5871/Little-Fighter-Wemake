#pragma once

#include <string>

#include "lfw/transform.h"
#include "lfw/ui/component/stage_dialog_listener.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/ui_img_loader.h"

namespace lfw {
namespace ui {

class UINode;

// TS `ui/component/Dialogs.ts`（TAGS = ["Dialogs"]）：对白框。
//   * props `head_node` / `text` / `talker` 都是 `UINode` 引用（端口走
//     `prop_instance` + `ref_to_node`，缺省 ⇒ nullptr）。
//   * `_listner`（StageDialogListener）跟「换关 + 对白变化」；`_head_loader` 的宿主是
//     `props.head_node` 那个节点（端口用 `UINodeImgAdapter` 适配）。
//   * `hide_dialog` 把 `_hiding` 置真 + scale_to(0,0,1) + 清文本/头像；`update` 里
//     transform 到位才真的隐藏（`set_visible(false)`）并把 transform 的缩放写回节点。
//   * `set_dialog` 末尾把世界 transform 挪到 `y = dialog ? 60 : 0`（对白上浮）。
class Dialogs : public UIComponent {
 public:
  static constexpr const char* TAG = "Dialogs";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  Dialogs(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  void on_start() override;
  void on_stop() override;
  void update(double dt) override;

  UINode* head_node();
  UINode* text_node();
  UINode* talker_node();

  bool hiding() const { return _hiding; }
  Transform& transform() { return _transform; }

  void hide_dialog();
  void show_dialog(const Value& dialog);
  void set_dialog(const Value& dialog);

 protected:
  void set_head_img(const std::u16string& path);

  StageDialogListener _listner;
  UINodeImgAdapter _head_adapter;
  UIImgLoader _head_loader;
  Transform _transform;
  bool _hiding = false;
};

}
}
