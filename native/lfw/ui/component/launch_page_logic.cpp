#include "lfw/ui/component/launch_page_logic.h"

#include <memory>
#include <optional>
#include <utility>

#include "lfw/base/clock.h"
#include "lfw/core/js_string.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/fade_out_opacity.h"
#include "lfw/ui/component/img_loop.h"
#include "lfw/ui/component/opacity_animation.h"
#include "lfw/ui/component/position_animation.h"
#include "lfw/ui/component/scale_animation.h"
#include "lfw/ui/component/sine_opacity.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

namespace {

// `node.find_component(X)`：TS 缺组件时会抛，用例保证存在；端口按「找不到跳过」处理。
template <typename T>
T* comp_of(UINode* n) {
  if (n == nullptr) return nullptr;
  UIComponent* const c = n->find_component(T::class_tag());
  return c != nullptr ? static_cast<T*>(c) : nullptr;
}

template <typename T>
T* comp_of_id(UINode* n, const std::u16string& id) {
  if (n == nullptr) return nullptr;
  UIComponent* const c = n->find_component_by_id(T::class_tag(), id);
  return c != nullptr ? static_cast<T*>(c) : nullptr;
}

template <typename T>
void enable_comp(UINode* n, bool on) {
  T* const c = comp_of<T>(n);
  if (c != nullptr) c->set_enabled(on);
}

// `c.start(true)` + `c.update(<最后一段时长>)`（TS 的 `anims[anims.length-1].duration`）。
void fade_finish(UINode* n) {
  OpacityAnimation* const c = comp_of<OpacityAnimation>(n);
  if (c == nullptr) return;
  c->start(true);
  const std::vector<Animation*>& anims = c->anim().anims();
  if (!anims.empty()) c->update(anims.back()->duration());
}

OpacityAnimation* first_opacity(UINode* n) { return comp_of<OpacityAnimation>(n); }

}

const ClazzTag* LaunchPageLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& LaunchPageLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"LaunchPage"};
  return tags;
}

const std::u16string& LaunchPageLogic::props_tag() const {
  static const std::u16string tag = u"LaunchPage";
  return tag;
}

const Value& LaunchPageLogic::props_meta() const {
  static const Value meta = [] {
    auto node_prop = [](bool nullable) {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      if (nullable) p->set(u"nullable", Value(true));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"entry", Value(u"string"));
    {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:ImgLoop"));
      o->set(u"loading_anim", Value(p));
    }
    o->set(u"tap_to_launch", Value(node_prop(false)));
    o->set(u"translation_note", Value(node_prop(true)));
    o->set(u"sound_warning", Value(node_prop(false)));
    o->set(u"yeonface", Value(node_prop(false)));
    o->set(u"bearface", Value(node_prop(false)));
    o->set(u"long_text", Value(node_prop(false)));
    return Value(o);
  }();
  return meta;
}

struct LaunchPageLogic::TapHintsState : IState {
  LaunchPageLogic& o;
  explicit TapHintsState(LaunchPageLogic& o) : o(o) {}
  Value key() const override { return Value(std::u16string(u"TapHints")); }
  void enter() override {
    ImgLoop* const la = o.loading_anim();
    if (la != nullptr) la->anim().set_times(o.prel_loaded() ? 1.0 : 0.0).set_count(0.0);
    enable_comp<SineOpacity>(o.tap_to_launch(), true);
    enable_comp<SineOpacity>(o.sound_warning(), true);
    enable_comp<FadeOutOpacity>(o.tap_to_launch(), false);
    enable_comp<FadeOutOpacity>(o.sound_warning(), false);
    enable_comp<SineOpacity>(o.translation_note(), true);
    enable_comp<FadeOutOpacity>(o.translation_note(), false);
  }
  void leave() override {
    enable_comp<SineOpacity>(o.tap_to_launch(), false);
    enable_comp<SineOpacity>(o.sound_warning(), false);
    enable_comp<FadeOutOpacity>(o.tap_to_launch(), true);
    enable_comp<FadeOutOpacity>(o.sound_warning(), true);
    enable_comp<SineOpacity>(o.translation_note(), false);
    enable_comp<FadeOutOpacity>(o.translation_note(), true);
  }
};

struct LaunchPageLogic::IntroductionState : IState {
  LaunchPageLogic& o;
  explicit IntroductionState(LaunchPageLogic& o) : o(o) {}
  Value key() const override { return Value(std::u16string(u"Introduction")); }
  void enter() override {
    lfw::timeout_add(
        [&o = o]() {
          o.lfw().host().sounds_play(Value(u"builtin_data/launch/093.wav.mp3"), Value(), Value(),
                                     Value());
        },
        1000.0);
    for (UINode* const n : {o.yeonface(), o.bearface()}) {
      ScaleAnimation* const sc = comp_of_id<ScaleAnimation>(n, u"scale_in");
      if (sc != nullptr) sc->start(false);
      PositionAnimation* const pos = comp_of_id<PositionAnimation>(n, u"move_in");
      if (pos != nullptr) pos->start(false);
      OpacityAnimation* const opa = first_opacity(n);
      if (opa != nullptr) opa->start(false);
    }
    {
      PositionAnimation* const pos = comp_of_id<PositionAnimation>(o.long_text(), u"move_in");
      if (pos != nullptr) pos->start(false);
      OpacityAnimation* const opa = comp_of_id<OpacityAnimation>(o.long_text(), u"opacity");
      if (opa != nullptr) opa->start(false);
    }
  }
  void leave() override {
    fade_finish(o.yeonface());
    fade_finish(o.bearface());
    {
      ScaleAnimation* const sc = comp_of_id<ScaleAnimation>(o.yeonface(), u"scale_out");
      if (sc != nullptr) sc->start(false);
    }
    {
      ScaleAnimation* const sc = comp_of_id<ScaleAnimation>(o.bearface(), u"scale_out");
      if (sc != nullptr) sc->start(false);
    }
    fade_finish(o.long_text());
  }
  std::optional<Value> update(double dt) override {
    (void)dt;
    if (!o.prel_loaded()) return std::nullopt;
    OpacityAnimation* const opa = first_opacity(o.long_text());
    if (opa != nullptr && opa->done()) return Value(std::u16string(u"GoToEntry"));
    return std::nullopt;
  }
};

struct LaunchPageLogic::GoToEntryState : IState {
  LaunchPageLogic& o;
  explicit GoToEntryState(LaunchPageLogic& o) : o(o) {}
  Value key() const override { return Value(std::u16string(u"GoToEntry")); }
  void enter() override { o.lfw().host().sounds_play_bgm(Value(u"bgm/main.wma.mp3")); }
  std::optional<Value> update(double dt) override {
    (void)dt;
    OpacityAnimation* const opa = first_opacity(o.long_text());
    if (opa != nullptr && opa->done()) {
      UILayer* const layer = o.node.layer();
      if (layer != nullptr) {
        UIPushPageOpts opts;
        const Value entry = o.props_holder.prop_value(u"entry");
        if (const std::u16string* const s = std::get_if<std::u16string>(&entry)) {
          opts.id = *s;
          opts.has_id = true;
        }
        layer->set(opts);
      }
      return Value(std::u16string(u"End"));
    }
    return std::nullopt;
  }
};

struct LaunchPageLogic::EndState : IState {
  LaunchPageLogic& o;
  explicit EndState(LaunchPageLogic& o) : o(o) {}
  Value key() const override { return Value(std::u16string(u"End")); }
  std::optional<Value> update(double dt) override {
    (void)dt;
    return std::nullopt;
  }
};

LaunchPageLogic::LaunchPageLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info), _fsm(u"LaunchPageLogic") {
  _states.push_back(std::make_unique<TapHintsState>(*this));
  _states.push_back(std::make_unique<IntroductionState>(*this));
  _states.push_back(std::make_unique<GoToEntryState>(*this));
  _states.push_back(std::make_unique<EndState>(*this));
  _fsm.add(_states[0].get(), _states[1].get(), _states[2].get(), _states[3].get());
}

ImgLoop* LaunchPageLogic::loading_anim() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"loading_anim"));
  return c != nullptr && is_class(c->clazz(), ImgLoop::class_tag()) ? static_cast<ImgLoop*>(c)
                                                                    : nullptr;
}

UINode* LaunchPageLogic::translation_note() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"translation_note"));
}

UINode* LaunchPageLogic::tap_to_launch() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"tap_to_launch"));
}

UINode* LaunchPageLogic::sound_warning() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"sound_warning"));
}

UINode* LaunchPageLogic::yeonface() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"yeonface"));
}

UINode* LaunchPageLogic::bearface() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"bearface"));
}

UINode* LaunchPageLogic::long_text() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"long_text"));
}

void LaunchPageLogic::on_prel_loaded() {
  _prel_loaded = true;
  ImgLoop* const la = loading_anim();
  if (la != nullptr) la->anim().set_times(4.0).set_count(0.0);
}

void LaunchPageLogic::on_start() {
  _prel_loaded = lfw().ui_loaded();
  lfw().host().sounds_load(Value(u"builtin_data/launch/093.wav.mp3"),
                           Value(u"builtin_data/launch/093.wav.mp3"));
  if (!_cb_remover) {
    _cb_remover = lfw().callbacks.on(u"on_prel_loaded",
                                     [this](const LfwCallbacks::Payloads&) { on_prel_loaded(); });
  }
}

void LaunchPageLogic::on_stop() {
  if (_cb_remover) {
    _cb_remover();
    _cb_remover = {};
  }
}

void LaunchPageLogic::on_resume() {
  if (lfw().host().is_desktop()) {
    UINode* const hint = node.find_child(u"desktop_hint");
    if (hint != nullptr) hint->set_visible(false);
  }
  _fsm.use(Value(std::u16string(u"TapHints")));
}

void LaunchPageLogic::on_key_down(LFWKeyEvent& e) {
  (void)e;
  on_tap();
}

void LaunchPageLogic::on_click(LFWPointerEvent& e) {
  (void)e;
  on_tap();
}

void LaunchPageLogic::on_tap() {
  IState* const s = _fsm.state();
  const Value key = s != nullptr ? s->key() : Value();
  if (equals(key, Value(std::u16string(u"TapHints")))) {
    _fsm.use(Value(std::u16string(u"Introduction")));
  }
}

void LaunchPageLogic::update(double dt) { _fsm.update(dt); }

}
}
