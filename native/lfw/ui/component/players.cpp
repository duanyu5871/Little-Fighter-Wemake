#include "lfw/ui/component/players.h"

#include "lfw/player_info.h"

namespace lfw {
namespace ui {

const ClazzTag* Players::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& Players::TAGS() {
  static const std::vector<std::u16string> tags{u"Players"};
  return tags;
}

double Players::max() { return props_holder.num(u"max").value_or(8.0); }

bool Players::player_join(PlayerInfo& player) {
  const double m = max();
  const double size = static_cast<double>(_players.size());
  if (size >= m) return false;
  _players.insert(&player);
  return true;
}

void Players::player_leave(PlayerInfo& player) { _players.erase(&player); }

}
}
