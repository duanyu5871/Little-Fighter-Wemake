#pragma once

#include <string>
#include <vector>

#include "lfw/ui/component/picture.h"

namespace lfw {

class Entity;

namespace ui {

// TS `ui/component/FighterThumb.ts`：`Picture` 子类，显示玩家角色的小头像
// （`fighter.data.base.small ?? CHARACTER_THUMB`）。
// ⚠ TS 的 `on_show` 从**兄弟组件** `PlayerScore` 取 `fighter`（`node.lookup_component`）；
// 端口**还没接线** ⇒ 先保持 `fighter = nullptr`（等价于"查不到 PlayerScore"）。
class FighterThumb : public Picture {
 public:
  static constexpr const char* TAG = "FighterThumb";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using Picture::Picture;

  const ClazzTag* clazz() const override { return class_tag(); }

  Entity* fighter = nullptr;
  std::u16string thumb_url();

  void on_start() override;
  void on_show() override;
  void handle_changed();
};

}
}
