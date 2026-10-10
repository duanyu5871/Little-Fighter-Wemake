#pragma once

#include <string>
#include <vector>

#include "lfw/ui/component/picture.h"

namespace lfw {

class PlayerInfo;

namespace ui {

// TS `ui/component/FighterHead.ts`：`Picture` 子类，取玩家角色的 `base.head`（缺省 RFACE）。
// ⚠ TS 的 `handle_changed()` **没有任何调用点**（死了），端口照抄不接线。
class FighterHead : public Picture {
 public:
  static constexpr const char* TAG = "FighterHead";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using Picture::Picture;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::u16string player_id();
  PlayerInfo* player();
  std::u16string head();

  void on_start() override;
  void handle_changed();
};

}
}
