#pragma once

#include <set>
#include <vector>

#include "lfw/ui/component/ui_component.h"

namespace lfw {

class PlayerInfo;

namespace ui {

// TS `ui/component/Players.ts`：props `{ max }`（缺省 8）；同组玩家容器（`Set<PlayerInfo>`）。
class Players : public UIComponent {
 public:
  static constexpr const char* TAG = "Players";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  const std::set<PlayerInfo*>& players() const { return _players; }
  double max();

  bool player_join(PlayerInfo& player);
  void player_leave(PlayerInfo& player);

 private:
  std::set<PlayerInfo*> _players;
};

}
}
