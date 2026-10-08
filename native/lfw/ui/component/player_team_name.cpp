#include "lfw/ui/component/player_team_name.h"

#include <memory>
#include <string>

#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

namespace {

Value make_style(bool com) {
  Value out(std::make_shared<Object>());
  Object* const o = as_object(out);
  o->set(u"fill_style", Value(std::u16string(com ? u"pink" : u"white")));
  o->set(u"font", Value(std::u16string(u"14px Arial")));
  return out;
}

}

const ClazzTag* PlayerTeamName::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerTeamName::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerTeamName"};
  return tags;
}

void PlayerTeamName::join(const std::u16string& text, bool com, bool decided) {
  _decided = decided;
  _com = com;
  node.set_text(text, make_style(com));
  node.set_visible(true);
}

void PlayerTeamName::quit() {
  _decided = false;
  _com = false;
  const std::u16string text = to_string(lfw().string(Value(std::u16string(u" "))));
  node.set_text(text, make_style(false));
  node.set_visible(false);
}

void PlayerTeamName::update(double dt) {
  _opacity.update(dt);
  node.set_opacity(_decided ? 1.0 : _opacity.value());
}

}
}
