#include "lfw/ui/component/dialogs.h"

#include "lfw/core/js_string.h"
#include "lfw/lfw.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

const Value& undef() {
  static const Value v;
  return v;
}

const Value* field_of(const Value& v, const char16_t* key) {
  const Object* const o = as_object(v);
  if (o == nullptr) return nullptr;
  return o->get(std::u16string(key));
}

// `v.base.name || ''` 这类取值：非字符串 ⇒ 空串（`|| ''` 的近似，数据里只可能是字符串）。
std::u16string text_of(const Value& v, const char16_t* key) {
  const Value* const p = field_of(v, key);
  if (p == nullptr) return std::u16string();
  const std::u16string* const s = std::get_if<std::u16string>(p);
  return s != nullptr ? *s : std::u16string();
}

}

const ClazzTag* Dialogs::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& Dialogs::TAGS() {
  static const std::vector<std::u16string> tags{u"Dialogs"};
  return tags;
}

const std::u16string& Dialogs::props_tag() const {
  static const std::u16string tag = u"Dialogs";
  return tag;
}

const Value& Dialogs::props_meta() const {
  static const Value meta = [] {
    auto prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"head_node", Value(prop()));
    o->set(u"text", Value(prop()));
    o->set(u"talker", Value(prop()));
    return Value(o);
  }();
  return meta;
}

Dialogs::Dialogs(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info),
      _listner(*this, [this](const Value& dialog, const Value& prev) {
        (void)prev;
        set_dialog(dialog);
      }),
      _head_adapter([this]() -> UINode* { return head_node(); }),
      _head_loader([this]() -> IUIImgLoaderNode* {
        return head_node() != nullptr ? static_cast<IUIImgLoaderNode*>(&_head_adapter) : nullptr;
      }) {}

UINode* Dialogs::head_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"head_node"));
}

UINode* Dialogs::text_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"text"));
}

UINode* Dialogs::talker_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"talker"));
}

void Dialogs::on_start() { _listner.start(); }

void Dialogs::on_stop() { _listner.stop(); }

void Dialogs::set_head_img(const std::u16string& path) {
  const UIImgLoadResult result = _head_loader.set_img(path);
  if (!result.ok) {
    // TS: `.catch(e => Ditto.warn('' + e))`（微任务 ⇒ 走 `warn_async`）
    const std::u16string text =
        result.error_is_error ? u"Error: " + result.error : result.error;
    lfw().host().warn_async({Value(text)});
  }
}

void Dialogs::hide_dialog() {
  _hiding = true;
  _transform.scale_to(0.0, 0.0, 1.0);
  if (UINode* const t = text_node()) t->set_text(u"");
  if (UINode* const tk = talker_node()) tk->set_text(u"");
  set_head_img(u"");
}

void Dialogs::show_dialog(const Value& dialog) {
  _hiding = false;
  node.set_visible(true);
  _transform.scale_to(1.0, 1.0, 1.0);
  if (UINode* const t = text_node()) t->set_text(text_of(dialog, u"i18n"));

  const Value* const fid = field_of(dialog, u"fighter");
  const Value* const fighter =
      (fid != nullptr && truthy(*fid)) ? lfw().datas().find_fighter(*fid) : nullptr;
  if (UINode* const tk = talker_node()) {
    if (fighter != nullptr) {
      // `fighter.base.name || ''`
      const Value* const base = field_of(*fighter, u"base");
      tk->set_text(base != nullptr ? text_of(*base, u"name") : std::u16string());
    } else {
      tk->set_text(u"");
    }
  }
  // `fighter?.base.head`：非字符串或空串都落到 `set_img('')`。
  std::u16string head;
  if (fighter != nullptr) {
    const Value* const base = field_of(*fighter, u"base");
    const Value* const h = base != nullptr ? field_of(*base, u"head") : nullptr;
    if (h != nullptr && truthy(*h)) {
      if (const std::u16string* const s = std::get_if<std::u16string>(h)) head = *s;
    }
  }
  set_head_img(head);
}

void Dialogs::update(double dt) {
  _transform.update(dt);
  if (_hiding && _transform.is_arrived()) {
    _hiding = false;
    node.set_visible(false);
  }
  node.set_scale(_transform.scale_x(), _transform.scale_y(), _transform.scale_z());
}

void Dialogs::set_dialog(const Value& dialog) {
  if (truthy(dialog)) {
    show_dialog(dialog);
  } else {
    hide_dialog();
  }
  world().transform.move_to(0.0, truthy(dialog) ? 60.0 : 0.0, 0.0);
}

}
}
