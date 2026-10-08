#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lfw/animation/sequence.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/StageTitleShow.ts`（TAGS = ["StageTitleShow"]）：换关/通关时闪一下标题，
// 透明度走 `Sequence(Easing(0,1)500, Delay(1)1500, Easing(1,0)500)`。
class StageTitleShow : public UIComponent {
 public:
  static constexpr const char* TAG = "StageTitleShow";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  StageTitleShow(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;
  void update(double dt) override;

  void set_text(const std::u16string& text);
  void on_chapter_finish();
  void on_stage_change(stage::Stage* stage, stage::Stage* prev);

 protected:
  Sequence _opacity;
  std::vector<std::unique_ptr<Animation>> _owned;
  std::function<void()> _world_remover;
  std::function<void()> _stage_remover;
};

}
}
