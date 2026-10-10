#pragma once

#include <string>
#include <vector>

#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/ComponentsPlayer.ts`（`IPlayable`）：按 props 拿到一组可播放组件，
// 统一转发 `start/stop/replay`。
//
// 照抄 TS 的 props 表怪癖：`components` 的 schema 键是 `"children"`、`recursive` 的 schema
// 键是 `"local"`（运行期读的还是 `raw.components` / `raw.recursive`，见 PROTOCOL）。
class ComponentsPlayer : public UIComponent {
 public:
  static constexpr const char* TAG = "ComponentsPlayer";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override;

  bool is_playable() const override { return true; }

  void on_start() override;
  void start() override;
  void stop() override;
  void replay() override;

  std::vector<UIComponent*> components;
};

}
}
