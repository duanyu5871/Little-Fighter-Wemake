#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "lfw/animation/sequence.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/component/ui_component_expression_builder.h"

namespace lfw {
namespace ui {

// TS `ui/component/FadeOutOpacity.ts`：args = [duration, delay]（缺省 1000 / 0）。
class FadeOutOpacity : public UIComponent {
 public:
  static constexpr const char* TAG = "FadeOutOpacity";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  // TS `static expression(duration = 1000, delay = 0)`（`Unsafe<number>` ⇒ 端口用 optional，
  // 空值拼成空串，与 TS 的 `duration ?? ''` 一致）。
  static UIComponentExpressionBuilder expression(std::optional<double> duration = 1000.0,
                                                 std::optional<double> delay = 0.0);

  void on_start() override;
  void update(double dt) override;

  void start(std::optional<bool> reverse = std::nullopt);

  Sequence& anim() { return _anim; }

 protected:
  Sequence _anim;
  std::vector<std::unique_ptr<Animation>> _owned;
};

}
}
