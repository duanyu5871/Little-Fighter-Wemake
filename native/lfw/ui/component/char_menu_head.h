#pragma once

#include <optional>
#include <string>

#include "lfw/animation/sine.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class GamePrepareLogic;
class Picture;
class UINode;

// TS `ui/component/CharMenuHead.ts`（TAGS = ["CharMenuHead"]）。
class CharMenuHead : public UIComponent {
 public:
  static constexpr const char* TAG = "CharMenuHead";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override { return PROPS(); }

  UINode* countdown_label();
  UINode* hints_node();
  Picture* head_pic();
  UINode* start_hints_node();
  UINode* pick_prev_node();
  UINode* pick_next_node();

  UINode* countdown_node();
  GamePrepareLogic* gpl();

  void join(const std::u16string& path);
  void quit();
  void update(double dt) override;
  void count_down(double num);

 protected:
  bool _joined = false;
  Sine _opacity{0.65, 1, 6};
  std::u16string _path;
  std::optional<double> _count_down_num;
};

}
}
