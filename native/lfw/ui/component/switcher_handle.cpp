#include "lfw/ui/component/switcher_handle.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

namespace {

UINode* prop_node(SwitcherHandle& me, const std::u16string& name) {
  if (!me.ensure_props()) return nullptr;
  return ref_to_node(me.props_holder.prop_instance(name));
}

// `child.text?.text !== key`（同 ArrowSwitcher）。
bool text_field_differs(UINode& n, const std::u16string& text) {
  const Object* const o = as_object(n.text());
  const Value* const v = o != nullptr ? o->get(u"text") : nullptr;
  if (v == nullptr) return true;
  const std::u16string* const s = std::get_if<std::u16string>(v);
  return s == nullptr || *s != text;
}

}

const ClazzTag* SwitcherHandle::class_tag() {
  static const ClazzTag tag{SliderHandle::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& SwitcherHandle::TAGS() {
  static const std::vector<std::u16string> tags{u"SwitcherHandle"};
  return tags;
}

const std::u16string& SwitcherHandle::props_tag() const {
  static const std::u16string tag = u"SwitcherHandle";
  return tag;
}

const Value& SwitcherHandle::props_meta() const {
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
    o->set(u"options", prop(u"$cls:UINode"));
    o->set(u"slots", prop(u"number"));
    return Value(o);
  }();
  return meta;
}

std::u16string SwitcherHandle::mode() {
  return props_holder.str(u"mode").value_or(u"switcher");
}

UINode* SwitcherHandle::options() { return prop_node(*this, u"options"); }

double SwitcherHandle::slots() {
  const std::optional<double> s = props_holder.num(u"slots");
  if (s.has_value()) return *s;
  UINode* const opts = options();
  return opts != nullptr ? static_cast<double>(opts->children().size()) : 0.0;
}

void SwitcherHandle::sync_range() {
  const double mx = max(static_cast<double>(items().size()) - 1.0, 0.0);
  if (min_value() != 0.0) set_min_value(0.0);
  if (max_value() != mx) set_max_value(mx);
  if (precision() != 1.0) set_precision(1.0);
  if (step() != 1.0) set_step(1.0);
  if (value() > max_value()) set_value(max_value());
}

double SwitcherHandle::cover_index() {
  UINode* const opts = options();
  if (opts == nullptr) return -1.0;
  const double n = static_cast<double>(items().size());
  if (n == 0.0) return -1.0;
  const bool row = direction() == u"row";
  const double track = row ? opts->w() : opts->h();
  if (!(track > 0.0)) return -1.0;
  const double cell = track / n;
  const UINode::Cross& t = opts->cross();
  // 第 0 格中心；手柄中心当前局部坐标 ⇒ 落在第 i 格即盖住该格。
  const double lo = (row ? t.left : t.top) + cell / 2;
  const double pos = row ? node.x() : node.y();
  const double idx = round((pos - lo) / cell);
  return max(0.0, min(n - 1.0, idx));
}

void SwitcherHandle::arrange_options(std::optional<double> cover) {
  UINode* const opts = options();
  if (opts == nullptr) return;
  const std::vector<std::u16string> its = items();
  const double n = static_cast<double>(its.size());
  if (n == 0.0) return;
  const bool row = direction() == u"row";
  const double track = row ? opts->w() : opts->h();
  if (!(track > 0.0)) return;
  const double cell = track / n;
  const UINode::Cross& t = opts->cross();
  const std::vector<UINode*>& children = opts->children();
  const double total = slots();
  const double covered = cover.has_value() ? *cover : cover_index();
  for (size_t i = 0; i < children.size() && static_cast<double>(i) < total; ++i) {
    UINode& child = *children[i];
    const bool show = static_cast<double>(i) < n && static_cast<double>(i) != covered;
    if (child.self_visible() != show) child.set_visible(show);
    if (!show) continue;
    const std::u16string& key = its[i];
    if (text_field_differs(child, key)) child.set_text(key);
    const UINode::Cross& c = child.cross();
    if (row) {
      const double center_x = t.left + (static_cast<double>(i) + 0.5) * cell;
      child.move_to(round(center_x - c.mid_x), round(t.mid_y - c.mid_y), child.z());
    } else {
      const double center_y = t.top + (static_cast<double>(i) + 0.5) * cell;
      child.move_to(round(t.mid_x - c.mid_x), round(center_y - c.mid_y), child.z());
    }
  }
}

void SwitcherHandle::on_start() {
  SliderHandle::on_start();
  sync_range();
  set_value(value());  // 刷新 handle_label 为当前项
  arrange_options(value());
}

void SwitcherHandle::on_resume() {
  SliderHandle::on_resume();
  sync_range();
  arrange_options(value());
}

void SwitcherHandle::update(double dt) {
  SliderHandle::update(dt);
  sync_range();
  arrange_options();
}

}
}
