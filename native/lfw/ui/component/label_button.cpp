#include "lfw/ui/component/label_button.h"

#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}
const ClazzTag* LabelButton::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& LabelButton::TAGS() {
  static const std::vector<std::u16string> tags{u"LabelButton"};
  return tags;
}

const std::u16string& LabelButton::props_tag() const {
  static const std::u16string tag = u"LabelButton";
  return tag;
}

const Value& LabelButton::props_meta() const {
  static const Value meta = [] {
    auto node_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto str_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"string"));
      p->set(u"nullable", Value(true));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"back_label", Value(node_prop()));
    o->set(u"hover_label", Value(node_prop()));
    o->set(u"responser", Value(node_prop()));
    o->set(u"normal_color", Value(str_prop()));
    o->set(u"hover_color", Value(str_prop()));
    o->set(u"disable_color", Value(str_prop()));
    o->set(u"text", Value(str_prop()));
    return Value(o);
  }();
  return meta;
}

LabelButton::LabelButton(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {
  anim.set_duration(150);
}

UINode* LabelButton::node_prop(const std::u16string& name) {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(name));
}

UINode& LabelButton::responser() {
  UINode* const r = node_prop(u"responser");
  return r != nullptr ? *r : node;
}

std::u16string LabelButton::normal_color() {
  const std::optional<std::u16string> v = props_holder.str(u"normal_color");
  return v.has_value() ? *v : std::u16string(u"#9ba5ce");
}

std::u16string LabelButton::hover_color() {
  const std::optional<std::u16string> v = props_holder.str(u"hover_color");
  return v.has_value() ? *v : std::u16string(u"white");
}

std::u16string LabelButton::text() const {
  UINode* const label = ref_to_node(const_cast<LabelButton*>(this)->props_holder.prop_instance(u"back_label"));
  if (label == nullptr) return std::u16string();
  const Value* const text = field_of(label->text(), u"text");
  return text != nullptr && !is_nullish(*text) ? to_string(*text) : std::u16string();
}

void LabelButton::set_text(const std::u16string& v) {
  UINode* const back = node_prop(u"back_label");
  if (back != nullptr) back->set_text(v);
  UINode* const hover = node_prop(u"hover_label");
  if (hover != nullptr) hover->set_text(v);
}

void LabelButton::update(double dt) {
  UINode& r = responser();
  bool reverse = false;
  if (is_nullish(_checked)) {
    reverse = (r.pointer_over() == 0 && !r.focused()) || r.pointer_down() != 0;
  } else {
    reverse = !truthy(_checked);
  }
  UINode* const hover = node_prop(u"hover_label");
  if (hover != nullptr) hover->set_opacity(anim.auto_trip(reverse, dt).value());
}

}
}
