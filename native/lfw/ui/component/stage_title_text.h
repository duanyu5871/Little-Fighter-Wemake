#pragma once

#include <functional>
#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/StageTitleText.ts`（TAGS = ["StageTitleText"]）：跟着世界换关与阶段切换，
// 把 `phase.title ?? stage.title ?? ''`（过 `lfw.string`）写上节点文本。
class StageTitleText : public UIComponent {
 public:
  static constexpr const char* TAG = "StageTitleText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;

  void set_stage(stage::Stage* stage);
  void set_phase(const Value& phase);
  void set_title(const std::u16string& title);

 protected:
  std::u16string _title;
  stage::Stage* _stage = nullptr;
  std::function<void()> _world_remover;
  std::function<void()> _stage_remover;
};

}
}
