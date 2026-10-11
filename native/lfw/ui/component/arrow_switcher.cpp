#include "lfw/ui/component/arrow_switcher.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/keys.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

namespace {

UINode* prop_node(ArrowSwitcher& me, const std::u16string& name) {
  if (!me.ensure_props()) return nullptr;
  return ref_to_node(me.props_holder.prop_instance(name));
}

// `node.text?.text !== text`（TS 的比较口径：节点文本对象里没有 `text` 字段、
// 或字段不是同一个字符串 ⇒ 不同）。
bool text_field_differs(UINode& n, const std::u16string& text) {
  const Object* const o = as_object(n.text());
  const Value* const v = o != nullptr ? o->get(u"text") : nullptr;
  if (v == nullptr) return true;
  const std::u16string* const s = std::get_if<std::u16string>(v);
  return s == nullptr || *s != text;
}

}

const ClazzTag* ArrowSwitcher::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& ArrowSwitcher::TAGS() {
  static const std::vector<std::u16string> tags{u"ArrowSwitcher"};
  return tags;
}

const std::u16string& ArrowSwitcher::props_tag() const {
  static const std::u16string tag = u"ArrowSwitcher";
  return tag;
}

const Value& ArrowSwitcher::props_meta() const {
  static const Value meta = [] {
    auto prop = [](const char16_t* type) {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(std::u16string(type)));
      p->set(u"nullable", Value(true));
      return Value(p);
    };
    auto o = std::make_shared<Object>();
    o->set(u"items", prop(u"string"));
    o->set(u"left", prop(u"$cls:UINode"));
    o->set(u"right", prop(u"$cls:UINode"));
    o->set(u"left_hl", prop(u"$cls:UINode"));
    o->set(u"right_hl", prop(u"$cls:UINode"));
    o->set(u"label", prop(u"$cls:UINode"));
    o->set(u"responser", prop(u"$cls:UINode"));
    o->set(u"left_text", prop(u"string"));
    o->set(u"right_text", prop(u"string"));
    return Value(o);
  }();
  return meta;
}

std::vector<std::u16string> ArrowSwitcher::items() {
  const std::optional<std::vector<std::u16string>> v = props_holder.strs(u"items");
  return v.has_value() ? *v : std::vector<std::u16string>();
}

double ArrowSwitcher::count() { return static_cast<double>(items().size()); }

double ArrowSwitcher::value() {
  const double n = count();
  return n != 0.0 ? min(_value, n - 1.0) : 0.0;
}

UINode* ArrowSwitcher::left() { return prop_node(*this, u"left"); }

UINode* ArrowSwitcher::right() { return prop_node(*this, u"right"); }

UINode* ArrowSwitcher::left_hl() { return prop_node(*this, u"left_hl"); }

UINode* ArrowSwitcher::right_hl() { return prop_node(*this, u"right_hl"); }

UINode* ArrowSwitcher::label() { return prop_node(*this, u"label"); }

UINode* ArrowSwitcher::responser() {
  if (UINode* const n = prop_node(*this, u"responser")) return n;
  return &node;
}

void ArrowSwitcher::on_value_changed(std::function<void(double, ArrowSwitcher&)> cb) {
  callbacks.on(u"on_value_changed", [cb](const ArrowSwitcherCallbacks::Payloads& ps) {
    for (const ArrowSwitcherCallbackArgs& a : ps) {
      if (a.self != nullptr) cb(a.value, *a.self);
    }
  });
}

void ArrowSwitcher::press(double dir) {
  responser()->set_focused(true);
  step(dir);
}

void ArrowSwitcher::step(double dir) {
  const double n = count();
  if (n <= 1.0) return;
  double c = _value + dir;
  // JS `((c % n) + n) % n`：负数也正确回卷。
  c = std::fmod(std::fmod(c, n) + n, n);
  if (c == _value) return;
  _value = c;
  sync_label();
  callbacks.call(u"on_value_changed", {ArrowSwitcherCallbackArgs{value(), this}});
}

void ArrowSwitcher::set_value(double v) {
  const double n = count();
  if (n == 0.0) return;
  // `Math.round(v) || 0`（NaN ⇒ 0）。
  double r = round(v);
  if (std::isnan(r)) r = 0.0;
  const double c = max(0.0, min(n - 1.0, r));
  if (c != _value) {
    _value = c;
    sync_label();
  }
}

void ArrowSwitcher::update_highlights(double dt) {
  UINode* const lh = left_hl();
  UINode* const rh = right_hl();
  const double k = min(1.0, dt / 90.0);
  if (lh != nullptr) {
    UINode* const l = left();
    const double t = l != nullptr && l->pointer_over() != 0 ? 1.0 : 0.0;
    _hl_l += (t - _hl_l) * k;
    lh->set_opacity(_hl_l);
  }
  if (rh != nullptr) {
    UINode* const r = right();
    const double t = r != nullptr && r->pointer_over() != 0 ? 1.0 : 0.0;
    _hl_r += (t - _hl_r) * k;
    rh->set_opacity(_hl_r);
  }
}

void ArrowSwitcher::on_start() {
  if (UINode* const l = left()) {
    l->callbacks.on_pointer_down = [this](LFWPointerEvent&, UINode&) { press(-1.0); };
  }
  if (UINode* const r = right()) {
    r->callbacks.on_pointer_down = [this](LFWPointerEvent&, UINode&) { press(1.0); };
  }
  sync_arrows();
  sync_label();
}

void ArrowSwitcher::on_resume() {
  sync_arrows();
  sync_label();
}

void ArrowSwitcher::on_stop() {
  if (UINode* const l = left()) l->callbacks.on_pointer_down = nullptr;
  if (UINode* const r = right()) r->callbacks.on_pointer_down = nullptr;
}

void ArrowSwitcher::update(double dt) {
  update_highlights(dt);

  UINode* const resp = responser();
  if (!(resp != nullptr && resp->focused()) && !node.focused()) return;
  Keys& ks = keys();
  // TS 的 `is_start()` 用 `ctrl.time`（= Keys.time）⇒ 显式传时间。
  if (ks.get(u"L")->is_start(ks.time())) step(-1.0);
  if (ks.get(u"R")->is_start(ks.time())) step(1.0);
}

void ArrowSwitcher::sync_label() {
  UINode* const l = label();
  if (l == nullptr) return;
  const std::vector<std::u16string> its = items();
  const double v = value();
  // `items[value] ?? ''`：数组下标不做截断（2.7 取不到），越界 ⇒ 空串。
  std::u16string key;
  if (!std::isnan(v) && std::trunc(v) == v && v >= 0.0 && v < static_cast<double>(its.size())) {
    key = its[static_cast<size_t>(v)];
  }
  if (text_field_differs(*l, key)) l->set_text(key);
}

void ArrowSwitcher::sync_arrows() {
  const std::u16string lt = props_holder.str(u"left_text").value_or(u"\u25c0");
  const std::u16string rt = props_holder.str(u"right_text").value_or(u"\u25b6");
  UINode* const l = left();
  if (l != nullptr && text_field_differs(*l, lt)) l->set_text(lt);
  UINode* const r = right();
  if (r != nullptr && text_field_differs(*r, rt)) r->set_text(rt);
  // 高亮叠层与箭头同字形。
  UINode* const lh = left_hl();
  if (lh != nullptr && text_field_differs(*lh, lt)) lh->set_text(lt);
  UINode* const rh = right_hl();
  if (rh != nullptr && text_field_differs(*rh, rt)) rh->set_text(rt);
}

}
}
