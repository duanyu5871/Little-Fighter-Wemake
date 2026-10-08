#pragma once

#include <string>

#include "lfw/ui/component/stage_dialog_listener.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/HideWhenDialoging.ts`（TAGS = ["HideWhenDialoging"]）：有对白时隐藏节点。
class HideWhenDialoging : public UIComponent {
 public:
  static constexpr const char* TAG = "HideWhenDialoging";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  HideWhenDialoging(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;
  void set_dialog(const Value& d);

  StageDialogListener& dialog_listener() { return _dialog_listener; }

 protected:
  // TS 成员初始化：`new StageDialogListener(this, d => this.set_dialog(d))`。
  StageDialogListener _dialog_listener{
      *this, [this](const Value& d, const Value&) { set_dialog(d); }};
};

}
}
