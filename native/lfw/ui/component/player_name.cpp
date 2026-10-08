#include "lfw/ui/component/player_name.h"

#include <memory>
#include <string>

#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

const ClazzTag* PlayerName::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerName::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerName"};
  return tags;
}

const std::u16string& PlayerName::props_tag() const {
  static const std::u16string tag = u"PlayerName";
  return tag;
}

const Value& PlayerName::props_meta() const {
  static const Value meta = [] {
    auto str_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"string"));
      p->set(u"nullable", Value(true));
      return Value(p);
    };
    auto bool_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"boolean"));
      p->set(u"nullable", Value(true));
      return Value(p);
    };
    auto o = std::make_shared<Object>();
    o->set(u"prefix", str_prop());
    o->set(u"hide_join_q", bool_prop());
    return Value(o);
  }();
  return meta;
}

Value PlayerName::make_style(bool com) {
  const Value& node_style = node.style.data();
  if (js_key_count(node_style) > 0) return copy_object(node_style);
  Value out(std::make_shared<Object>());
  Object* const o = as_object(out);
  o->set(u"fill_style", Value(std::u16string(com ? u"pink" : u"white")));
  o->set(u"font", Value(std::u16string(u"14px Arial")));
  return out;
}

void PlayerName::join(const std::u16string& text, bool com, bool decided) {
  (void)props();
  const std::optional<std::u16string> prefix = props_holder.str(u"prefix");
  _decided = decided;
  _com = com;
  _decided = true;
  const std::u16string t =
      prefix.has_value() ? to_string(lfw().string(Value(*prefix))) + text : text;
  node.set_text(t, make_style(com));
  node.set_visible(true);
}

void PlayerName::quit() {
  _decided = false;
  _com = false;
  const std::u16string text =
      to_string(lfw().string(Value(std::u16string(u"char_menu.join_q"))));
  node.set_text(text, make_style(false));
  (void)props();
  if (props_holder.bool_(u"hide_join_q").value_or(false)) node.set_visible(false);
}

void PlayerName::update(double dt) {
  _opacity.update(dt);
  node.set_opacity(_decided ? 1.0 : _opacity.value());
}

}
}
