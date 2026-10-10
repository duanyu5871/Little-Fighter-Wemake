#pragma once

#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/StageSwitcher.ts`：按 `switch_stage(_r)` 广播在 `lfw.datas.stages` 里
// 循环换关（缺省 `VOID_STAGE`）；列表按作弊码（LF2_NET/GIM_INK）过 `hidden`/`Dev`/`is_starting` 三道筛。
class StageSwitcher : public UIComponent {
 public:
  static constexpr const char* TAG = "StageSwitcher";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  bool show_all();
  std::vector<Value> stages();
  const Value& stage() const { return _stage; }
  std::u16string stage_text();

  void on_resume() override;
  void on_pause() override;
  void on_show() override;

  void on_broadcast(const std::u16string& v);

  // TS 成员初值：`_stage = Defines.VOID_STAGE`（同一性比较要靠同一个对象）。
  static Value default_stage();
  Value _stage = default_stage();
  LfwCallbacks::Remover _remover;
};

}
}
