#include "lfw/ui/component/slider_handle.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/core/js_string.h"
#include "lfw/defines/defines_data.h"
#include "lfw/keys.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace ui {

namespace {

// `Defines.MODERN_SCREEN_WIDTH/HEIGHT`（走运行时表，与 TS 同一个值；`camera.cpp` 同口径）。
double screen_width() { return defines::num(u"Defines.MODERN_SCREEN_WIDTH"); }
double screen_height() { return defines::num(u"Defines.MODERN_SCREEN_HEIGHT"); }

// JS 真值判定（`NaN` / `0` 为假）——`factor` 可能是 `NaN`（0/0 除出来的）。
bool num_truthy(double v) { return v != 0.0 && !std::isnan(v); }

// `arr.at(idx)`：ToIntegerOrInfinity 截断、负数从尾部数；越界/±∞ ⇒ 空（TS 的 undefined）。
std::optional<std::u16string> array_at(const std::vector<std::u16string>& a, double idx) {
  double i = std::isnan(idx) ? 0.0 : std::trunc(idx);
  if (i < 0.0) i += static_cast<double>(a.size());
  if (i < 0.0 || i >= static_cast<double>(a.size())) return std::nullopt;
  return a[static_cast<size_t>(i)];
}

// 节点型属性：`prop_instance` + `ref_to_node`（同 Dialogs）。
UINode* prop_node(SliderHandle& me, const std::u16string& name) {
  if (!me.ensure_props()) return nullptr;
  return ref_to_node(me.props_holder.prop_instance(name));
}

}

const ClazzTag* SliderHandle::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& SliderHandle::TAGS() {
  static const std::vector<std::u16string> tags{u"SliderHandle"};
  return tags;
}

const std::u16string& SliderHandle::props_tag() const {
  static const std::u16string tag = u"SliderHandle";
  return tag;
}

const Value& SliderHandle::props_meta() const {
  static const Value meta = [] {
    auto prop = [](const char16_t* type) {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(std::u16string(type)));
      p->set(u"nullable", Value(true));
      return Value(p);
    };
    auto dir_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"string"));
      auto oneof = std::make_shared<Array>();
      oneof->push_back(Value(u"row"));
      oneof->push_back(Value(u"col"));
      p->set(u"oneof", Value(std::move(oneof)));
      p->set(u"nullable", Value(true));
      return Value(p);
    };
    auto o = std::make_shared<Object>();
    o->set(u"mode", prop(u"string"));
    o->set(u"min", prop(u"number"));
    o->set(u"max", prop(u"number"));
    o->set(u"precision", prop(u"number"));
    o->set(u"step", prop(u"number"));
    o->set(u"container", prop(u"$cls:UINode"));
    o->set(u"responser", prop(u"$cls:UINode"));
    o->set(u"items_container", prop(u"$cls:UINode"));
    o->set(u"handle_label", prop(u"$cls:UINode"));
    o->set(u"items", prop(u"string"));
    o->set(u"visible_items", prop(u"number"));
    o->set(u"direction", dir_prop());
    return Value(o);
  }();
  return meta;
}

std::u16string SliderHandle::direction() {
  return props_holder.str(u"direction", {u"row", u"col"}).value_or(u"row");
}

std::u16string SliderHandle::mode() { return props_holder.str(u"mode").value_or(u"default"); }

void SliderHandle::set_mode(const std::u16string& v) { props_holder.set_str(u"mode", Value(v)); }

std::vector<std::u16string> SliderHandle::items() {
  const std::optional<std::vector<std::u16string>> v = props_holder.strs(u"items");
  return v.has_value() ? *v : std::vector<std::u16string>();
}

void SliderHandle::set_items(const std::vector<std::u16string>& v) {
  // TS `props.items = v.join()`（缺省分隔符 ","）。
  std::u16string joined;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i != 0) joined += u",";
    joined += v[i];
  }
  props_holder.set_str(u"items", Value(std::move(joined)));
}

UINode* SliderHandle::container() {
  if (UINode* const n = prop_node(*this, u"container")) return n;
  return node.parent();
}

UINode* SliderHandle::responser() {
  if (UINode* const n = prop_node(*this, u"responser")) return n;
  return container();
}

UINode* SliderHandle::handle_label() { return prop_node(*this, u"handle_label"); }

UINode* SliderHandle::items_container() { return prop_node(*this, u"items_container"); }

double SliderHandle::step() { return props_holder.num(u"step").value_or(10.0); }

void SliderHandle::set_step(double v) { props_holder.set_num(u"step", Value(v)); }

double SliderHandle::precision() { return props_holder.num(u"precision").value_or(1.0); }

void SliderHandle::set_precision(double v) { props_holder.set_num(u"precision", Value(v)); }

double SliderHandle::min_value() { return props_holder.num(u"min").value_or(0.0); }

void SliderHandle::set_min_value(double v) { props_holder.set_num(u"min", Value(v)); }

double SliderHandle::max_value() { return props_holder.num(u"max").value_or(100.0); }

void SliderHandle::set_max_value(double v) { props_holder.set_num(u"max", Value(v)); }

double SliderHandle::factor() { return clamp(_factor, 0.0, 1.0); }

void SliderHandle::set_factor(double v) {
  _factor = clamp(v, 0.0, 1.0);
  UINode* const label = handle_label();
  if (label == nullptr) return;
  if (mode() == u"switcher") {
    const std::vector<std::u16string> its = items();
    if (!its.empty()) {
      const std::optional<std::u16string> at = array_at(its, value());
      label->set_text(at.has_value() ? *at : number_to_string(value()));
      return;
    }
  }
  label->set_text(number_to_string(value()));
}

double SliderHandle::value() {
  const double mn = min_value();
  const double mx = max_value();
  return round_float(mn + (mx - mn) * _factor, precision());
}

void SliderHandle::set_value(double v) {
  const double mn = min_value();
  const double mx = max_value();
  v = round_float(clamp(v, mn, mx), precision());
  // TS 里 getter 会再读一遍（同一个值），端口照抄两段取值。
  const double mn2 = min_value();
  const double mx2 = max_value();
  set_factor(round_float((v - mn2) / (mx2 - mn2)));
}

void SliderHandle::on_value_changed(std::function<void(double, SliderHandle&)> cb) {
  callbacks.on(u"on_value_changed", [cb](const SliderHandleCallbacks::Payloads& ps) {
    for (const SliderHandleCallbackArgs& a : ps) {
      if (a.self != nullptr) cb(a.value, *a.self);
    }
  });
}

void SliderHandle::handle_pointing_event(const LfwPointingEvent& e) {
  UINode* const cont = container();
  if (cont == nullptr) return;
  const UINode::Geo& geo = cont->geo();
  const UINode::Cross& cross = node.cross();

  if (direction() == u"row") {
    // 场景 x ∈ [-1,1] → 屏幕 px（TS `Defines.MODERN_SCREEN_WIDTH * (e.scene_x + 1) / 2`）。
    const double fx = screen_width() * (e.scene_x + 1) / 2;
    const double min_num = geo.left - geo.pos_x - cross.left;
    const double max_num = geo.right - geo.pos_x - cross.right;
    const double x = clamp(fx - geo.pos_x, min_num, max_num);
    set_factor(round_float((x - min_num) / (max_num - min_num)));
    set_value(value());
  } else {
    const double fy = screen_height() * (1 - e.scene_y) / 2;
    const double min_num = geo.top - geo.pos_y - cross.top;
    const double max_num = geo.bottom - geo.pos_y - cross.bottom;
    const double y = clamp(fy - geo.pos_y, min_num, max_num);
    set_factor(round_float((y - min_num) / (max_num - min_num)));
    set_value(value());
  }
}

void SliderHandle::on_p_down(LFWPointerEvent& e) {
  (void)e;
  UINode* const resp = responser();
  UINode* const cont = container();
  if (resp != nullptr) resp->set_focused(true);
  else if (cont != nullptr) cont->set_focused(true);
  else return;
  lfw().pointings_grab(this);
  // `min==0 && max==1 && precision==1 && step==1` = 开关：直接翻转，不进拖动态。
  if (min_value() == 0.0 && max_value() == 1.0 && precision() == 1.0 && step() == 1.0) {
    set_factor(num_truthy(_factor) ? 0.0 : 1.0);
    callbacks.call(u"on_value_changed", {SliderHandleCallbackArgs{value(), this}});
  } else {
    _on_me = true;
  }
}

void SliderHandle::PointListener::on_pointer_down(LfwPointingEvent& e) {
  if (!owner->_on_me) return;
  owner->handle_pointing_event(e);
}

void SliderHandle::PointListener::on_pointer_move(LfwPointingEvent& e) {
  if (!owner->_on_me) return;
  owner->handle_pointing_event(e);
}

void SliderHandle::PointListener::on_pointer_up(LfwPointingEvent& e) {
  SliderHandle& me = *owner;
  if (me._on_me) {
    me.handle_pointing_event(e);
    me.callbacks.call(u"on_value_changed", {SliderHandleCallbackArgs{me.value(), &me}});
    me._on_me = false;
  }
  me.lfw().pointings_ungrab(&me);
}

void SliderHandle::PointListener::on_pointer_cancel(LfwPointingEvent& e) {
  (void)e;
  SliderHandle& me = *owner;
  if (me._on_me) {
    me.callbacks.call(u"on_value_changed", {SliderHandleCallbackArgs{me.value(), &me}});
    me._on_me = false;
  }
  me.lfw().pointings_ungrab(&me);
}

void SliderHandle::on_start() {
  if (UINode* const cont = container()) {
    cont->callbacks.on_pointer_down = [this](LFWPointerEvent& e, UINode&) { on_p_down(e); };
  }
  lfw().pointings_callback_add(&_b);
  if (mode() == u"switcher") {
    set_max_value(max(static_cast<double>(items().size()) - 1.0, 0.0));
    set_min_value(0.0);
    set_precision(1.0);
    set_step(1.0);
  }
}

void SliderHandle::on_stop() {
  if (UINode* const cont = container()) cont->callbacks.on_pointer_down = nullptr;
  lfw().pointings_callback_del(&_b);
  lfw().pointings_ungrab(this);
}

void SliderHandle::update(double dt) {
  (void)dt;
  UINode* const cont = container();
  if (cont == nullptr) return;
  const UINode::Cross& cross = node.cross();
  const UINode::Geo& geo = cont->geo();
  UINode* const parent = node.parent();

  if (parent != nullptr) {
    const std::u16string m = mode();
    const Vector3& size = parent->size;
    if (m == u"switcher") {
      const size_t length = items().size();
      if (length != 0) {
        if (direction() == u"row") node.resize(size.x / static_cast<double>(length), size.y);
        else node.resize(size.x, size.y / static_cast<double>(length));
      }
    } else if (m == u"default") {
      if (direction() == u"row") node.set_h(size.y);
      else node.set_w(size.x);
    } else if (m == u"items_scroll_bar") {
      UINode* const ic = items_container();
      const double kids = ic != nullptr ? static_cast<double>(ic->children().size()) : 0.0;
      // `children.length || 1`：0（与 undefined）都取 1。
      const double length = kids == 0.0 ? 1.0 : kids;
      const double visibles = min(props_holder.num(u"visible_items").value_or(length), length);
      if (direction() == u"row") node.resize(size.x * visibles / length, size.y);
      else node.resize(size.x, size.y * visibles / length);
    }
  }

  // 手柄位置向目标点缓动（无条件，先于焦点门）。
  if (direction() == u"row") {
    const double min_num = geo.left - geo.pos_x - cross.left;
    const double max_num = geo.right - geo.pos_x - cross.right;
    const double dist = min_num + (max_num - min_num) * factor();
    const double curr = node.pos.x + (dist - node.pos.x) * 0.25;
    node.set_x(round(clamp(curr, min_num, max_num)));
  } else {
    const double min_num = geo.top - geo.pos_y - cross.top;
    const double max_num = geo.bottom - geo.pos_y - cross.bottom;
    const double dist = min_num + (max_num - min_num) * factor();
    const double curr = node.pos.y + (dist - node.pos.y) * 0.25;
    node.set_y(round(clamp(curr, min_num, max_num)));
  }

  UINode* const resp = responser();
  if (!(resp != nullptr && resp->focused()) && !cont->focused() && !node.focused()) return;

  const std::u16string m = mode();
  if (m == u"default") {
    const double lr = LR();
    const double ud = UD();
    if (direction() == u"row" && num_truthy(lr)) {
      set_value(value() + step() * lr);
    } else if (direction() == u"col" && num_truthy(ud)) {
      set_value(value() - step() * ud);
    }
  } else if (direction() == u"row") {
    Keys& ks = keys();
    // TS 的 `is_start()` 用 `ctrl.time`（= Keys.time = lfw 生命周期）⇒ 显式传时间。
    const double lr = (ks.get(u"R")->is_start(ks.time()) ? 1.0 : 0.0) -
                      (ks.get(u"L")->is_start(ks.time()) ? 1.0 : 0.0);
    const double prev = value();
    double curr = value() + step() * lr;
    if (curr < min_value()) curr = max_value();
    if (curr > max_value()) curr = min_value();
    set_value(curr);
    if (prev != curr) {
      callbacks.call(u"on_value_changed", {SliderHandleCallbackArgs{value(), this}});
    }
  } else if (direction() == u"col") {
    Keys& ks = keys();
    const double ud = (ks.get(u"D")->is_start(ks.time()) ? 1.0 : 0.0) -
                      (ks.get(u"U")->is_start(ks.time()) ? 1.0 : 0.0);
    const double prev = value();
    double curr = value() - step() * ud;
    if (curr < min_value()) curr = max_value();
    if (curr > max_value()) curr = min_value();
    set_value(curr);
    if (prev != curr) {
      callbacks.call(u"on_value_changed", {SliderHandleCallbackArgs{value(), this}});
    }
  }
}

}
}
