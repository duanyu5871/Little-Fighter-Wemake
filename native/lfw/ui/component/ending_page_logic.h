#pragma once

#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/jalousie.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/EndingPageLogic.ts`（TAGS = ["EndingPageLogic"]）：通关结算页
// （百叶窗逐条翻结算文案，翻完回主菜单或退到首页）。
class EndingPageLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "EndingPageLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  EndingPageLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }
  const Value& props_meta() const override { return PROPS(); }

  void on_start() override;
  void on_resume() override;
  void on_stop() override;
  void on_click(LFWPointerEvent& e) override;
  void on_key_down(LFWKeyEvent& e) override;

  void on_jalousie_anim_end(Jalousie& j);

  UINode* label_node();
  Jalousie* jalousie();

 private:
  struct JalousieCbT : JalousieCallbacks::Listener {
    explicit JalousieCbT(EndingPageLogic* o);
  };

  double difficulty();

  double texts_idx_ = 0;
  std::vector<std::u16string> texts_{
      u"old_ending_0_0", u"old_ending_0_1", u"old_ending_1",  u"old_ending_2",
      u"old_ending_3",   u"gim_ending_0",   u"gim_ending_1",   u"gim_ending_2",
      u"gim_ending_3",
  };
  JalousieCbT _cb{this};
};

}
}
