#include "lfw/ui/component/scroll_view.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/defines/defines_data.h"
#include "lfw/keys.h"
#include "lfw/ui/component/slider_handle.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {
namespace ui {

namespace {

// `Defines.MODERN_SCREEN_WIDTH/HEIGHT`（走运行时表，与 TS 同一个值；`slider_handle.cpp` 同口径）。
double screen_width() { return defines::num(u"Defines.MODERN_SCREEN_WIDTH"); }
double screen_height() { return defines::num(u"Defines.MODERN_SCREEN_HEIGHT"); }

// JS 真值判定（`NaN` / `0` 为假）。
bool num_truthy(double v) { return v != 0.0 && !std::isnan(v); }

// 节点型属性：`prop_instance` + `ref_to_node`（同 SliderHandle）。
UINode* prop_node(ScrollView& me, const std::u16string& name) {
  if (!me.ensure_props()) return nullptr;
  return ref_to_node(me.props_holder.prop_instance(name));
}

// 组件型属性：`prop_instance` + `ref_to_comp`。
UIComponent* prop_comp(ScrollView& me, const std::u16string& name) {
  if (!me.ensure_props()) return nullptr;
  return ref_to_comp(me.props_holder.prop_instance(name));
}

constexpr double KEY_REPEAT_MS = 80;

}

const ClazzTag* ScrollView::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& ScrollView::TAGS() {
  static const std::vector<std::u16string> tags{u"ScrollView"};
  return tags;
}

const std::u16string& ScrollView::props_tag() const {
  static const std::u16string tag = u"ScrollView";
  return tag;
}

const Value& ScrollView::props_meta() const {
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
    o->set(u"direction", dir_prop());
    o->set(u"content", prop(u"$cls:UINode"));
    o->set(u"scrollbar", prop(u"$cls:SliderHandle"));
    o->set(u"scrollbar_thumb", prop(u"$cls:UINode"));
    o->set(u"draggable", prop(u"boolean"));
    o->set(u"keyboard", prop(u"boolean"));
    o->set(u"wheel", prop(u"boolean"));
    o->set(u"cull", prop(u"boolean"));
    o->set(u"step", prop(u"number"));
    o->set(u"speed", prop(u"number"));
    o->set(u"drag_threshold", prop(u"number"));
    o->set(u"wheel_speed", prop(u"number"));
    o->set(u"follow_focus", prop(u"boolean"));
    return Value(o);
  }();
  return meta;
}

std::u16string ScrollView::direction() {
  return props_holder.str(u"direction", {u"row", u"col"}).value_or(u"col");
}

bool ScrollView::is_col() { return direction() == u"col"; }

double ScrollView::step() { return props_holder.num(u"step").value_or(40.0); }

double ScrollView::speed() {
  const std::optional<double> v = props_holder.num(u"speed");
  return v.has_value() ? *v : 0.4;
}

bool ScrollView::draggable() { return props_holder.bool_(u"draggable").value_or(true); }

bool ScrollView::keyboard() { return props_holder.bool_(u"keyboard").value_or(true); }

bool ScrollView::wheel_enabled() { return props_holder.bool_(u"wheel").value_or(true); }

double ScrollView::wheel_speed() { return props_holder.num(u"wheel_speed").value_or(1.0); }

bool ScrollView::cull() { return props_holder.bool_(u"cull").value_or(false); }

double ScrollView::drag_threshold() {
  return props_holder.num(u"drag_threshold").value_or(8.0);
}

bool ScrollView::follow_focus() {
  return props_holder.bool_(u"follow_focus").value_or(true);
}

UINode* ScrollView::content() {
  if (_content != nullptr) return _content;
  if (UINode* const c = prop_node(*this, u"content")) return _content = c;
  const std::vector<UINode*>& kids = node.children();
  UINode* child = nullptr;
  for (UINode* const v : kids) {
    if (v->self_visible()) {
      child = v;
      break;
    }
  }
  if (child == nullptr && !kids.empty()) child = kids[0];
  return _content = child;
}

void ScrollView::set_content(UINode* v) {
  _content = v;
  _captured = false;
  _target_offset = 0;
  _offset = 0;
}

UINode* ScrollView::scrollbar_thumb() {
  if (_sb_thumb != nullptr) return _sb_thumb;
  if (UINode* const p = prop_node(*this, u"scrollbar_thumb")) return _sb_thumb = p;
  const std::u16string sid = props_holder.str(u"scrollbar_thumb").value_or(u"scroll_thumb");
  UINode* found = node.root().search_node(sid);
  if (found == nullptr) {
    IUiLayers* const layers = lfw().ui_layers();
    UINode* const ui_root = layers != nullptr ? layers->ui() : nullptr;
    if (ui_root != nullptr) found = ui_root->search_node(sid);
  }
  if (found != nullptr) _sb_thumb = found;
  return found;
}

SliderHandle* ScrollView::scrollbar() {
  if (_scrollbar != nullptr) return _scrollbar;
  if (UIComponent* const c = prop_comp(*this, u"scrollbar")) {
    if (!lfw::is_class(c->clazz(), SliderHandle::class_tag())) return nullptr;
    return _scrollbar = static_cast<SliderHandle*>(c);
  }
  const std::optional<std::u16string> sid = props_holder.str(u"scrollbar");
  if (!sid.has_value()) return nullptr;
  UIComponent* const found =
      node.root().search_component_by_id(SliderHandle::class_tag(), *sid);
  if (found != nullptr) _scrollbar = static_cast<SliderHandle*>(found);
  return _scrollbar;
}

double ScrollView::max_scroll() {
  const std::pair<double, double> r = range();
  return max(0.0, r.second - r.first);
}

bool ScrollView::dragging() { return _dragging || _sb_dragging; }

double ScrollView::offset() { return _offset; }

void ScrollView::set_offset(double v) { scroll_to(v); }

double ScrollView::factor() {
  const double m = max_scroll();
  return m > 0 ? clamp(_offset / m, 0.0, 1.0) : 0.0;
}

void ScrollView::set_factor(double v) { scroll_to(clamp(v, 0.0, 1.0) * max_scroll()); }

void ScrollView::on_start() {
  // 启用 overflow:hidden：视口矩形外的内容（渲染与指针）都会被裁剪。
  node.set_clip_children(true);
  lfw().pointings_callback_add(&_p);
  _captured = false;
  _target_offset = _offset;
}

void ScrollView::on_stop() {
  lfw().pointings_callback_del(&_p);
  _armed = false;
  _dragging = false;
}

void ScrollView::on_resume() {
  node.set_clip_children(true);
  _armed = false;
  _dragging = false;
}

ScrollView& ScrollView::scroll_to(double off) {
  _target_offset = clamp(off, 0.0, max_scroll());
  return *this;
}

ScrollView& ScrollView::scroll_by(double delta) {
  return scroll_to(_target_offset + delta);
}

ScrollView& ScrollView::scroll_to_start() { return scroll_to(0.0); }

ScrollView& ScrollView::scroll_to_end() { return scroll_to(max_scroll()); }

void ScrollView::on_scroll(std::function<void(double, double, ScrollView&)> cb) {
  callbacks.on(u"on_scroll", [cb](const ScrollViewCallbacks::Payloads& ps) {
    for (const ScrollViewCallbackArgs& a : ps) {
      if (a.self != nullptr) cb(a.offset, a.factor, *a.self);
    }
  });
}

bool ScrollView::ensure_visible(UINode* target) {
  if (target == nullptr) return false;
  const double m = max_scroll();
  if (m <= 0) return false;
  const std::pair<double, double> r = node_range(target);
  const double view = is_col() ? node.h() : node.w();
  const double win_lo = window_lo();
  const double win_hi = win_lo + view;
  double o = _target_offset;
  if (r.first < win_lo) {
    o += r.first - win_lo;
  } else if (r.second > win_hi) {
    o += r.second - win_hi;
  }
  if (o != _target_offset) scroll_to(o);
  return o != _target_offset;
}

void ScrollView::update(double dt) {
  UINode* const c = content();
  if (c == nullptr) return;
  if (!_captured) {
    _base_x = c->x();
    _base_y = c->y();
    _captured = true;
  }

  // 内容尺寸变化后收敛目标。
  _target_offset = clamp(_target_offset, 0.0, max_scroll());
  if (_dragging || _sb_dragging || speed() <= 0 || speed() >= 1) {
    _offset = _target_offset;
  } else {
    _offset += (_target_offset - _offset) * speed();
    if (abs(_target_offset - _offset) < 0.5) _offset = _target_offset;
  }

  // 拖拽中不抢焦点跟随（否则 ensure_visible 会与拖拽互相拉扯）。
  if (follow_focus() && !_dragging && !_sb_dragging) {
    UINode* const foc = focused_in(c);
    if (foc != nullptr) ensure_visible(foc);
  }

  apply_offset();
  sync_scrollbar();  // 尽早同步滚动条（内容一变就更新 thumb）。
  update_cull();
  update_keys(dt);

  const double o = round(_offset);
  if (o != _reported) {
    _reported = o;
    callbacks.call(u"on_scroll", {ScrollViewCallbackArgs{_offset, factor(), this}});
  }
}

std::pair<double, double> ScrollView::range() {
  UINode* const c = content();
  if (c == nullptr) return {0.0, 0.0};
  if (is_col()) {
    const double vh = node.h();
    if (vh <= 0) return {0.0, 0.0};
    const double vt = node.cross().top;
    const std::pair<double, double> b = box(c);
    if (b.second - b.first <= vh) return {0.0, 0.0};
    return {b.first - vt + _base_y, b.second - (vt + vh) + _base_y};
  }
  const double vw = node.w();
  if (vw <= 0) return {0.0, 0.0};
  const double vl = node.cross().left;
  const std::pair<double, double> b = box(c);
  if (b.second - b.first <= vw) return {0.0, 0.0};
  return {b.first - vl + _base_x, b.second - (vl + vw) + _base_x};
}

double ScrollView::window_lo() {
  const double lo = range().first;
  UINode* const c = content();
  if (c == nullptr) return _offset;
  if (is_col()) {
    return node.cross().top - _base_y + lo + _offset;
  }
  return node.cross().left - _base_x + lo + _offset;
}

std::pair<double, double> ScrollView::box(UINode* c) {
  double lo = 0;
  double hi = 0;
  bool first = true;
  for (UINode* const child : c->children()) {
    if (!child->self_visible() && !is_culled(child)) continue;
    const std::pair<double, double> r = node_range(child);
    if (first) {
      lo = r.first;
      hi = r.second;
      first = false;
    } else {
      lo = min(lo, r.first);
      hi = max(hi, r.second);
    }
  }
  return {lo, hi};
}

std::pair<double, double> ScrollView::node_range(UINode* n) {
  UINode* const c = content();
  double ox = 0;
  double oy = 0;
  for (UINode* p = n; p != nullptr && p != c; p = p->parent()) {
    ox += p->x();
    oy += p->y();
  }
  if (is_col()) {
    return {oy + n->cross().top, oy + n->cross().bottom};
  }
  return {ox + n->cross().left, ox + n->cross().right};
}

void ScrollView::apply_offset() {
  UINode* const c = content();
  if (c == nullptr) return;
  const double t = range().first + _offset;
  if (is_col()) {
    const double y = _base_y - t;
    if (c->y() != y) c->set_y(y);
  } else {
    const double x = _base_x - t;
    if (c->x() != x) c->set_x(x);
  }
}

void ScrollView::update_cull() {
  if (!cull()) return;
  UINode* const c = content();
  if (c == nullptr) return;
  const double view = is_col() ? node.h() : node.w();
  if (view <= 0) return;
  const double win_lo = window_lo();
  const double win_hi = win_lo + view;
  for (UINode* const child : c->children()) {
    const std::pair<double, double> r = node_range(child);
    const bool inside = r.second > win_lo && r.first < win_hi;
    if (inside) {
      const std::vector<UINode*>::iterator it =
          std::find(_culled.begin(), _culled.end(), child);
      if (it != _culled.end()) {
        _culled.erase(it);
        const std::map<UINode*, bool>::iterator oit = _cull_orig.find(child);
        const bool orig = oit != _cull_orig.end() ? oit->second : true;
        if (child->self_visible() != orig) child->set_visible(orig);
      }
    } else if (!is_culled(child) && child->self_visible()) {
      if (_cull_orig.find(child) == _cull_orig.end()) _cull_orig[child] = true;
      _culled.push_back(child);
      child->set_visible(false);
    }
  }
}

bool ScrollView::is_culled(UINode* n) const {
  return std::find(_culled.begin(), _culled.end(), n) != _culled.end();
}

// ===================== 方向键 =====================

double ScrollView::key_dir() {
  auto hold = [this](const char16_t* key) {
    controller::KeyStatus* const st = keys().get(key);
    return st != nullptr && !st->is_end();
  };
  if (is_col()) {
    return (hold(u"D") ? 1.0 : 0.0) - (hold(u"U") ? 1.0 : 0.0);
  }
  return (hold(u"R") ? 1.0 : 0.0) - (hold(u"L") ? 1.0 : 0.0);
}

void ScrollView::update_keys(double dt) {
  if (!keyboard() || max_scroll() <= 0) {
    _key_active = false;
    _key_time = 0;
    return;
  }
  UINode* const c = content();
  if (!(node.focused() || (c != nullptr && c->focused()))) {
    _key_active = false;
    _key_time = 0;
    return;
  }
  const double dir = key_dir();
  if (!num_truthy(dir)) {
    _key_active = false;
    _key_time = 0;
    return;
  }
  if (!_key_active) {
    _key_active = true;
    _key_time = 0;
    scroll_by(dir * step());
    return;
  }
  _key_time += dt;
  while (_key_time >= KEY_REPEAT_MS) {
    _key_time -= KEY_REPEAT_MS;
    scroll_by(dir * step());
  }
}

// 焦点节点是否位于内容子树内。

UINode* ScrollView::focused_in(UINode* c) {
  UINode* const f = node.focused_node();
  if (f == nullptr || f == &node || f == c) return nullptr;
  for (UINode* n = f; n != nullptr; n = n->parent()) {
    if (n == c) return f;
  }
  return nullptr;
}

// 让内容子树内持有焦点的节点失焦（拖拽开始时调用，避免松开后被焦点跟随吸回）。

void ScrollView::unfocus_content() {
  UINode* const c = content();
  if (c == nullptr) return;
  UINode* const f = focused_in(c);
  if (f != nullptr) f->set_focused(false);
}

double ScrollView::pointer_axis_px(const LfwPointingEvent& e) {
  if (is_col()) {
    return screen_height() * (1 - e.scene_y) / 2;
  }
  return screen_width() * (e.scene_x + 1) / 2;
}

bool ScrollView::pointer_in_view(const LfwPointingEvent& e) {
  const UINode::Geo& g = node.geo();
  if (is_col()) {
    const double fy = screen_height() * (1 - e.scene_y) / 2;
    return fy >= g.top && fy <= g.bottom;
  }
  const double fx = screen_width() * (e.scene_x + 1) / 2;
  return fx >= g.left && fx <= g.right;
}

// 指针是否落在某节点矩形内（双向判定，便于命中轨道）。

bool ScrollView::pointer_in_node(const LfwPointingEvent& e, UINode* n) {
  const UINode::Geo& g = n->geo();
  const double fx = screen_width() * (e.scene_x + 1) / 2;
  const double fy = screen_height() * (1 - e.scene_y) / 2;
  return fx >= g.left && fx <= g.right && fy >= g.top && fy <= g.bottom;
}

// 由滚动条轨道上的指针位置换算进度（考虑抓取偏移 _sb_grab）。

void ScrollView::scrollbar_pointer(const LfwPointingEvent& e) {
  UINode* const thumb = scrollbar_thumb();
  UINode* const track = thumb != nullptr ? thumb->parent() : nullptr;
  if (thumb == nullptr || track == nullptr) return;
  const double m = max_scroll();
  if (m <= 0) return;
  const double len = is_col() ? thumb->h() : thumb->w();
  const double tlen = is_col() ? track->h() : track->w();
  const double travel = tlen - len;
  if (travel <= 0) return;
  const UINode::Geo& g = track->geo();
  const double start = is_col() ? g.top : g.left;
  // thumb 起始端 = 指针 - 抓取偏移；轨道点击时 _sb_grab=len/2 → thumb 居中于指针。
  const double p = pointer_axis_px(e) - _sb_grab;
  const double f = clamp((p - start) / travel, 0.0, 1.0);
  set_factor(f);
  _offset = _target_offset;  // 拖动时即时跟随。
  apply_offset();
  sync_scrollbar();  // thumb 立刻跟到指针下。
}

void ScrollView::PointListener::on_pointer_down(LfwPointingEvent& e) {
  ScrollView& me = *owner;
  if (!me.node.visible() || me.node.disabled()) return;
  if (e.button != 0) return;
  if (me.max_scroll() <= 0) return;
  // 滚动条轨道优先（轨道在视口外/右，需先于内容拖拽判定）。
  UINode* const thumb = me.scrollbar_thumb();
  UINode* const track = thumb != nullptr ? thumb->parent() : nullptr;
  if (thumb != nullptr && track != nullptr && me.pointer_in_node(e, track)) {
    me.unfocus_content();
    me._sb_dragging = true;
    const double axis = me.pointer_axis_px(e);
    if (me.pointer_in_node(e, thumb)) {
      // 点中 thumb 本体：只"抓住"，保持指针相对 thumb 起始端的偏移，不立即滚动。
      const UINode::Geo& tg = thumb->geo();
      const double cur_start = me.is_col() ? tg.top : tg.left;
      me._sb_grab = axis - cur_start;
    } else {
      // 点中轨道空白：跳到指针处（指针=thumb 中心）。
      const double len = me.is_col() ? thumb->h() : thumb->w();
      me._sb_grab = len / 2;
      me.scrollbar_pointer(e);
    }
    return;
  }
  // 内容拖拽仅限真正的视口矩形内（含横向范围）。
  if (!me.draggable()) return;
  if (!me.pointer_in_node(e, &me.node)) return;
  // 子控件（如音量滑条）已抢占本次手势时，内容拖拽让位。
  if (me.lfw().pointings_grabbing() != nullptr) return;
  me._armed = true;
  me._dragging = false;
  me._drag_dist = 0;
  me._last_axis = me.pointer_axis_px(e);
}

void ScrollView::PointListener::on_pointer_move(LfwPointingEvent& e) {
  ScrollView& me = *owner;
  if (me._sb_dragging) {
    me.scrollbar_pointer(e);
    return;
  }
  if (!me._armed) return;
  const double px = me.pointer_axis_px(e);
  const double d = px - me._last_axis;
  me._last_axis = px;
  if (!me._dragging) {
    me._drag_dist += abs(d);
    if (me._drag_dist < me.drag_threshold()) return;
    me._dragging = true;
    me.unfocus_content();
    me.callbacks.call(u"on_scroll_start", {ScrollViewCallbackArgs{me._offset, me.factor(), &me}});
  }
  // 内容跟随指针移动。
  me._target_offset = clamp(me._target_offset - d, 0.0, me.max_scroll());
  me._offset = me._target_offset;
  me.apply_offset();
  me.sync_scrollbar();
}

void ScrollView::PointListener::on_pointer_up(LfwPointingEvent& e) {
  (void)e;
  owner->end_drag();
}

void ScrollView::PointListener::on_pointer_cancel(LfwPointingEvent& e) {
  (void)e;
  owner->end_drag();
}

void ScrollView::PointListener::on_wheel(LfwPointingEvent& e) {
  ScrollView& me = *owner;
  if (!me.wheel_enabled()) return;
  if (!me.node.visible() || me.node.disabled()) return;
  if (me._armed || me._dragging || me._sb_dragging) return;
  if (me.max_scroll() <= 0) return;
  // 只有光标真正位于视口矩形内时才滚动（不含右侧滚动条）。
  if (!me.pointer_in_node(e, &me.node)) return;
  // 主方向增量优先，其次使用另一轴：
  // 例如横向列表可用普通鼠标的"垂直滚轮"滚动（Shift/触控板横滑时系统会把增量放到其中一轴）。
  const double dx = e.delta_x;
  const double dy = e.delta_y;
  const double d = me.is_col() ? (num_truthy(dy) ? dy : dx) : (num_truthy(dx) ? dx : dy);
  if (!num_truthy(d)) return;
  me.scroll_by(d * me.wheel_speed());
}

void ScrollView::end_drag() {
  if (_dragging) {
    callbacks.call(u"on_scroll_end", {ScrollViewCallbackArgs{_offset, factor(), this}});
  }
  _armed = false;
  _dragging = false;
  _sb_dragging = false;
}

void ScrollView::sync_scrollbar() {
  // 直接受控的 thumb 节点优先（尺寸+位置都由本组件负责，不依赖 SliderHandle）。
  if (UINode* const thumb = scrollbar_thumb()) {
    sync_thumb(thumb);
    return;
  }
  SliderHandle* const sb = scrollbar();
  if (sb == nullptr) return;
  if (sb->min_value() != 0) sb->set_min_value(0);
  if (sb->max_value() != 100) sb->set_max_value(100);
  if (!_last_slider_factor.has_value()) {
    _last_slider_factor = sb->factor();
  } else if (sb->factor() != *_last_slider_factor) {
    _last_slider_factor = sb->factor();
    set_factor(sb->factor());
  }
  const double want = factor();
  if (abs(sb->factor() - want) > 1e-4) sb->set_factor(want);
  _last_slider_factor = sb->factor();

  UINode& sbt = sb->node;
  UINode* const track = sbt.parent();
  if (track == nullptr) return;
  const double view = is_col() ? node.h() : node.w();
  const double m = max_scroll();
  if (!(view > 0)) return;
  const double track_len = is_col() ? track->h() : track->w();
  if (!(track_len > 0)) return;
  const double content_len = view + m;
  const double ratio = content_len > 0 ? view / content_len : 1;
  const double min_len = min(20.0, track_len);
  const double len = round(clamp(track_len * ratio, min_len, track_len));
  if (is_col()) {
    if (sbt.h() != len) sbt.resize(sbt.w(), len);
  } else {
    if (sbt.w() != len) sbt.resize(len, sbt.h());
  }
}

// 直接驱动滚动条 thumb：尺寸 = 轨道 × 视口/内容；位置随 factor（内容进度）。
// 轨道 = thumb 的父节点；坐标系取各自 cross（thumb 居中与否都成立）。

void ScrollView::sync_thumb(UINode* thumb) {
  UINode* const track = thumb->parent();
  if (track == nullptr) return;
  const double view = is_col() ? node.h() : node.w();
  const double m = max_scroll();
  if (!(view > 0)) return;
  const double track_len = is_col() ? track->h() : track->w();
  if (!(track_len > 0)) return;

  // 尺寸。
  const double content_len = view + m;
  const double ratio = content_len > 0 ? clamp(view / content_len, 0.0, 1.0) : 1;
  const double min_len = min(20.0, track_len);
  const double len = round(clamp(track_len * ratio, min_len, track_len));
  if (is_col()) {
    if (thumb->h() != len) thumb->resize(thumb->w(), len);
  } else {
    if (thumb->w() != len) thumb->resize(len, thumb->h());
  }

  // 位置：thumb 顶端 = 轨道顶端 + 行程 × factor。
  const double travel = track_len - len;
  if (travel <= 0) return;
  const double f = factor();
  const double top =
      (is_col() ? track->cross().top : track->cross().left) + travel * f;
  // 节点 pos（track 局部）= rect 顶端 - cross.axis。
  const double pos = top - (is_col() ? thumb->cross().top : thumb->cross().left);
  if (is_col()) {
    const double y = round(pos);
    if (thumb->y() != y) thumb->set_y(y);
  } else {
    const double x = round(pos);
    if (thumb->x() != x) thumb->set_x(x);
  }
}

}
}
