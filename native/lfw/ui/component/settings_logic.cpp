#include "lfw/ui/component/settings_logic.h"

#include <string>
#include <vector>

#include "lfw/ui/component/label_button.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"

namespace lfw {
namespace ui {

namespace {

LabelButton* label_button_of(UINode* n) {
  if (n == nullptr) return nullptr;
  UIComponent* const c = n->search_component(LabelButton::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), LabelButton::class_tag()) ? static_cast<LabelButton*>(c) : nullptr;
}

}

const ClazzTag* SettingsLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& SettingsLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"SettingsLogic"};
  return tags;
}

const Value& SettingsLogic::PROPS() {
  static const Value meta = [] {
    auto node_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"btn_misc_settings", Value(node_prop()));
    o->set(u"btn_ctrl_settings", Value(node_prop()));
    o->set(u"misc_settings_node", Value(node_prop()));
    o->set(u"ctrl_settings_node", Value(node_prop()));
    return Value(o);
  }();
  return meta;
}

SettingsLogic::SettingsLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {}

UINode* SettingsLogic::btn_misc_settings() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"btn_misc_settings"));
}

UINode* SettingsLogic::btn_ctrl_settings() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"btn_ctrl_settings"));
}

UINode* SettingsLogic::misc_settings_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"misc_settings_node"));
}

UINode* SettingsLogic::ctrl_settings_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"ctrl_settings_node"));
}

void SettingsLogic::show_misc() {
  UINode* const bm = btn_misc_settings();
  UINode* const bc = btn_ctrl_settings();
  LabelButton* const lm = label_button_of(bm);
  LabelButton* const lc = label_button_of(bc);
  if (lm != nullptr) lm->set_checked(Value(true));
  if (lc != nullptr) lc->set_checked(Value(false));
  UINode* const mn = misc_settings_node();
  UINode* const cn = ctrl_settings_node();
  if (mn != nullptr) mn->set_visible(true);
  if (cn != nullptr) cn->set_visible(false);
}

void SettingsLogic::show_ctrl() {
  UINode* const bm = btn_misc_settings();
  UINode* const bc = btn_ctrl_settings();
  LabelButton* const lm = label_button_of(bm);
  LabelButton* const lc = label_button_of(bc);
  if (lm != nullptr) lm->set_checked(Value(false));
  if (lc != nullptr) lc->set_checked(Value(true));
  UINode* const mn = misc_settings_node();
  UINode* const cn = ctrl_settings_node();
  if (mn != nullptr) mn->set_visible(false);
  if (cn != nullptr) cn->set_visible(true);
}

void SettingsLogic::on_start() {
  UINode* const bm = btn_misc_settings();
  UINode* const bc = btn_ctrl_settings();
  LabelButton* const lm = label_button_of(bm);
  LabelButton* const lc = label_button_of(bc);
  if (lm != nullptr) lm->set_checked(Value(true));
  if (lc != nullptr) lc->set_checked(Value(false));
  if (bm != nullptr) bm->callbacks.on_click = [this](LFWPointerEvent&) { show_misc(); };
  if (bc != nullptr) bc->callbacks.on_click = [this](LFWPointerEvent&) { show_ctrl(); };
}

void SettingsLogic::on_stop() {
  UINode* const bm = btn_misc_settings();
  UINode* const bc = btn_ctrl_settings();
  if (bm != nullptr) bm->callbacks.on_click = nullptr;
  if (bc != nullptr) bc->callbacks.on_click = nullptr;
}

}
}
