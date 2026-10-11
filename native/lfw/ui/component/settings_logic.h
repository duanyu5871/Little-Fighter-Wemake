#pragma once

#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/SettingsLogic.ts`（TAGS = ["SettingsLogic"]）：设置页
// 「杂项 / 按键」两个分页按钮的互斥高亮 + 面板显隐。
class SettingsLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "SettingsLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  SettingsLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const Value& props_meta() const override { return PROPS(); }

  void on_start() override;
  void on_stop() override;

  UINode* btn_misc_settings();
  UINode* btn_ctrl_settings();
  UINode* misc_settings_node();
  UINode* ctrl_settings_node();

 private:
  void show_misc();
  void show_ctrl();
};

}
}
