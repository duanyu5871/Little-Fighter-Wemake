#pragma once

#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/BackgroundSwitcher.ts`：按 `switch_background(_r)` / `reset_background`
// 广播在可选背景表里循环（表 = `RANDOM_BG` + `lfw.datas.backgrounds` 按作弊码过滤，末尾剔掉 `VOID_BG`）。
class BackgroundSwitcher : public UIComponent {
 public:
  static constexpr const char* TAG = "BackgroundSwitcher";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::vector<Value> backgrounds();
  const Value& background() const { return _background; }

  void on_resume() override;
  void on_pause() override;
  void on_show() override;

  void on_broadcast(const std::u16string& v);

  // TS 成员初值：`_background = Defines.RANDOM_BG`。
  static Value default_background();
  Value _background = default_background();
  LfwCallbacks::Remover _remover;
};

}
}
