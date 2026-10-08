#include "lfw/ui/component/fighter_name.h"

#include <memory>
#include <string>

#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

const ClazzTag* FighterName::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& FighterName::TAGS() {
  static const std::vector<std::u16string> tags{u"FighterName"};
  return tags;
}

Value FighterName::make_style(bool com) {
  const Value& node_style = node.style.data();
  if (js_key_count(node_style) > 0) return copy_object(node_style);
  Value out(std::make_shared<Object>());
  Object* const o = as_object(out);
  o->set(u"fill_style", Value(std::u16string(com ? u"pink" : u"white")));
  o->set(u"font", Value(std::u16string(u"14px Arial")));
  return out;
}

void FighterName::join(const std::u16string& text, bool com, bool decided) {
  _decided = decided;
  _com = com;
  node.set_text(text, make_style(com));
  node.set_visible(true);
}

void FighterName::quit() {
  _com = false;
  _decided = false;
  const std::u16string text = to_string(lfw().string(Value(std::u16string(u" "))));
  node.set_text(text, make_style(false));
  node.set_visible(false);
}

void FighterName::update(double dt) {
  _opacity.update(dt);
  node.set_opacity(_decided ? 1.0 : _opacity.value());
}

}
}
