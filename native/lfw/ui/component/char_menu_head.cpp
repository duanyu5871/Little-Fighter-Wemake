#include "lfw/ui/component/char_menu_head.h"

#include <memory>

#include "lfw/core/js_string.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/game_prepare_logic.h"
#include "lfw/ui/component/picture.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

const ClazzTag* CharMenuHead::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& CharMenuHead::TAGS() {
  static const std::vector<std::u16string> tags{u"CharMenuHead"};
  return tags;
}

const std::u16string& CharMenuHead::props_tag() const {
  static const std::u16string tag = u"CharMenuHead";
  return tag;
}

const Value& CharMenuHead::PROPS() {
  static const Value props = [] {
    auto cls_prop = [](const char16_t* name) {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:" + std::u16string(name)));
      return p;
    };
    auto node_prop = [&] {
      auto p = cls_prop(u"UINode");
      p->set(u"nullable", Value(true));
      return p;
    };
    auto o = std::make_shared<Object>();
    auto countdown_label = cls_prop(u"UINode");
    countdown_label->set(u"nullable", Value(false));
    o->set(u"countdown_label", Value(countdown_label));
    auto hints_node = cls_prop(u"UINode");
    hints_node->set(u"nullable", Value(false));
    o->set(u"hints_node", Value(hints_node));
    auto head_pic = cls_prop(u"Picture");
    head_pic->set(u"nullable", Value(true));
    o->set(u"head_pic", Value(head_pic));
    o->set(u"start_hints_node", Value(node_prop()));
    o->set(u"pick_prev_node", Value(node_prop()));
    o->set(u"pick_next_node", Value(node_prop()));
    return Value(o);
  }();
  return props;
}

UINode* CharMenuHead::countdown_label() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"countdown_label"));
}

UINode* CharMenuHead::hints_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"hints_node"));
}

Picture* CharMenuHead::head_pic() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"head_pic"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), Picture::class_tag()) ? static_cast<Picture*>(c) : nullptr;
}

UINode* CharMenuHead::start_hints_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"start_hints_node"));
}

UINode* CharMenuHead::pick_prev_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"pick_prev_node"));
}

UINode* CharMenuHead::pick_next_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"pick_next_node"));
}

UINode* CharMenuHead::countdown_node() { return node.find_child(u"countdown_text"); }

GamePrepareLogic* CharMenuHead::gpl() {
  UIComponent* const c = node.root().find_component(GamePrepareLogic::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), GamePrepareLogic::class_tag()) ? static_cast<GamePrepareLogic*>(c)
                                                             : nullptr;
}

void CharMenuHead::join(const std::u16string& path) {
  _joined = true;
  _path = path;
  Picture* const pic = head_pic();
  if (pic != nullptr) {
    pic->node.set_visible(true);
    pic->set_src(path);
  }
  UINode* const hints = hints_node();
  if (hints != nullptr) hints->set_visible(false);
  UINode* const cd = countdown_node();
  if (cd != nullptr) cd->set_visible(false);
}

void CharMenuHead::quit() {
  _joined = false;
  _path.clear();
  Picture* const pic = head_pic();
  if (pic != nullptr) pic->node.set_visible(false);
  UINode* const hints = hints_node();
  if (hints != nullptr) hints->set_visible(true);
  UINode* const cd = countdown_node();
  if (cd != nullptr) cd->set_visible(false);
}

void CharMenuHead::update(double dt) {
  _opacity.update(dt);
  UINode* const cd = countdown_node();
  const bool cd_visible = cd != nullptr && cd->visible();
  const bool hints_visible = !_joined && !cd_visible;
  UINode* const hints = hints_node();
  if (hints != nullptr) {
    hints->set_visible(hints_visible);
    hints->set_opacity(_opacity.value());
  }
  UINode* const start_hints = start_hints_node();
  if (start_hints != nullptr) start_hints->set_visible(_joined && !cd_visible);
  const bool pick_visible = _joined && !cd_visible;
  UINode* const prev = pick_prev_node();
  if (prev != nullptr) prev->set_visible(pick_visible);
  UINode* const next = pick_next_node();
  if (next != nullptr) next->set_visible(pick_visible);
  Picture* const pic = head_pic();
  if (pic != nullptr) {
    pic->node.set_visible(!hints_visible && !cd_visible && !_path.empty());
  }
}

void CharMenuHead::count_down(double num) {
  num = floor(num);
  if (!_count_down_num.has_value() || num != *_count_down_num) {
    _count_down_num = num;
    UINode* const label = countdown_label();
    if (label != nullptr) label->set_text(number_to_string(num));
  }
  UINode* const cd = countdown_node();
  if (cd != nullptr) cd->set_visible(num >= 1);
}

}
}
