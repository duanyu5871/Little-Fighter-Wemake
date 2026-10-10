#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/ui/component/ui_component.h"

namespace lfw {

class PlayerInfo;
class World;

namespace ui {

class UINode;

// TS `ui/component/Alignment.ts`（原文 FIXME：「总觉得这玩意设计有问题 -Gim」）。
// props：`followed`（字符串或 `{l,r,t,b}`）、`follower`、`left`/`right`/`top`/`bottom`、
// `offset`（`{l,r,t,b}`）、`once`。
class Alignment : public UIComponent {
 public:
  static constexpr const char* TAG = "Alignment";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  struct Followeds {
    UINode* l = nullptr;
    UINode* r = nullptr;
    UINode* t = nullptr;
    UINode* b = nullptr;
  };
  struct Aligns {
    std::optional<std::u16string> l;
    std::optional<std::u16string> r;
    std::optional<std::u16string> t;
    std::optional<std::u16string> b;
  };
  struct Offsets {
    double l = 0.0;
    double r = 0.0;
    double t = 0.0;
    double b = 0.0;
  };

  UINode* follower = nullptr;
  Followeds followed;
  Aligns align;
  Offsets offset;
  bool once = false;

  void on_start() override;
  void update(double dt) override;
};

}
}
