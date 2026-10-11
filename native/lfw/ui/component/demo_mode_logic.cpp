#include "lfw/ui/component/demo_mode_logic.h"

#include <cmath>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/bg/background.h"
#include "lfw/camera.h"
#include "lfw/core/js_string.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/stage_actions.h"
#include "lfw/defines/stage_group.h"
#include "lfw/entity/entity.h"
#include "lfw/helper/randoming.h"
#include "lfw/lfw.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/player_info.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/camera_ctrl.h"
#include "lfw/ui/component/components_player.h"
#include "lfw/ui/component/fighter_stat_bar.h"
#include "lfw/ui/component/jalousie.h"
#include "lfw/ui/component/mode_state.h"
#include "lfw/ui/component/weapon_rain.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/ui_event.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/range.h"

namespace lfw {
namespace ui {

namespace {

// TS 的四个静态字段（单进程一份；绑首个调用者的 `mt`，与 TS 静态成员同行为）。
struct DemoStatics {
  std::unique_ptr<Randoming> situations;
  Value situation = Value();  // `_situation: DemoSituation | null = null`
  std::unique_ptr<Randoming> stages;
};

DemoStatics& demo_statics() {
  static DemoStatics s;
  return s;
}

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// JS `false != v.group?.some(x => x != g)`：nullish/非数组 ⇒ true（`false != undefined`）。
bool group_some_ne(const Value& group_field, const char16_t* g) {
  const Array* const arr = as_array(group_field);
  if (arr == nullptr) return true;
  for (const Value& item : arr->items()) {
    if (!strict_equals(item, Value(std::u16string(g)))) return true;
  }
  return false;
}

Value vstr(const char16_t* s) { return Value(std::u16string(s)); }

Value sit(const std::u16string& title, bool stage_mode, std::vector<Value> teams,
          std::vector<Value> oids = {}) {
  auto o = std::make_shared<Object>();
  o->set(u"title", Value(title));
  o->set(u"stage_mode", Value(stage_mode));
  o->set(u"teams", Value(std::make_shared<Array>(std::move(teams))));
  if (!oids.empty()) o->set(u"oids", Value(std::make_shared<Array>(std::move(oids))));
  return Value(o);
}

std::vector<Value> teams_fill(double n, const char16_t* v) {
  std::vector<Value> out;
  for (double i = 0; i < n; ++i) out.push_back(vstr(v));
  return out;
}

std::vector<Value> cat(std::vector<Value> a, std::vector<Value> b) {
  a.insert(a.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
  return a;
}

// `range(1, n).map(v => '' + v)`。
std::vector<Value> teams_range(double n) {
  std::vector<Value> out;
  const std::optional<std::vector<double>> nums = range(1.0, n);
  if (!nums.has_value()) return out;
  for (const double v : *nums) out.push_back(Value(to_string(Value(v))));
  return out;
}

std::vector<Value> oids_of(std::initializer_list<const char16_t*> list) {
  std::vector<Value> out;
  for (const char16_t* s : list) out.push_back(vstr(s));
  return out;
}

// `stage?.bg ?? '?'`（nullish 才回退）。
std::u16string stage_bg_id(const Value& stage) {
  if (is_nullish(stage)) return u"?";
  const Value bg = field_or(stage, u"bg");
  if (is_nullish(bg)) return u"?";
  return to_string(bg);
}

// `stage.id ?? ''`。
std::u16string stage_id_or_empty(const Value& stage) {
  const Value id = field_or(stage, u"id");
  if (is_nullish(id)) return std::u16string();
  return to_string(id);
}

// `bg?.id || '?'`（falsy 含空串都回退）。
std::u16string bg_id_or_question(const Value& bg) {
  if (is_nullish(bg)) return u"?";
  const Value id = field_or(bg, u"id");
  return truthy(id) ? to_string(id) : u"?";
}

void stop_gogogo(ComponentsPlayer* p) {
  if (p == nullptr) return;
  p->stop();
  p->node.set_visible(false);
  p->node.set_opacity(0);
}

}

const ClazzTag* DemoModeLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& DemoModeLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"DemoModeLogic"};
  return tags;
}

const Value& DemoModeLogic::PROPS() {
  static const Value meta = [] {
    auto cls_prop = [](const char16_t* name) {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:" + std::u16string(name)));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"focus_prefix", Value(cls_prop(u"UINode")));
    o->set(u"focus_on", Value(cls_prop(u"UINode")));
    o->set(u"cam_ctrl", Value(cls_prop(u"CameraCtrl")));
    o->set(u"score_board", Value(cls_prop(u"UINode")));
    o->set(u"situation_name", Value(cls_prop(u"UINode")));
    o->set(u"focus_text_node", Value(cls_prop(u"UINode")));
    o->set(u"jalousie", Value(cls_prop(u"Jalousie")));
    o->set(u"gogogo", Value(cls_prop(u"ComponentsPlayer")));
    o->set(u"gogogo_loop", Value(cls_prop(u"ComponentsPlayer")));
    return Value(o);
  }();
  return meta;
}

DemoModeLogic::DemoModeLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info), _fsm(u"DemoFSM") {
  _states.push_back(std::make_unique<GameModeFSMState_Running>(*this));
  _states.push_back(std::make_unique<GameModeFSMState_BeforeEnd>(*this));
  _states.push_back(std::make_unique<GameModeFSMState_End>(*this));
  _fsm.add(_states[0].get(), _states[1].get(), _states[2].get());
}

UINode* DemoModeLogic::focus_prefix() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"focus_prefix"));
}

UINode* DemoModeLogic::focus_on() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"focus_on"));
}

CameraCtrl* DemoModeLogic::cam_ctrl() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"cam_ctrl"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), CameraCtrl::class_tag()) ? static_cast<CameraCtrl*>(c) : nullptr;
}

UINode* DemoModeLogic::score_board() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"score_board"));
}

UINode* DemoModeLogic::situation_name() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"situation_name"));
}

UINode* DemoModeLogic::focus_text_node() {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(u"focus_text_node"));
}

Jalousie* DemoModeLogic::jalousie() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"jalousie"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), Jalousie::class_tag()) ? static_cast<Jalousie*>(c) : nullptr;
}

ComponentsPlayer* DemoModeLogic::gogogo() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"gogogo"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), ComponentsPlayer::class_tag()) ? static_cast<ComponentsPlayer*>(c)
                                                             : nullptr;
}

ComponentsPlayer* DemoModeLogic::gogogo_loop() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"gogogo_loop"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), ComponentsPlayer::class_tag()) ? static_cast<ComponentsPlayer*>(c)
                                                             : nullptr;
}

Randoming& DemoModeLogic::get_situations(LFW& lfw) {
  DemoStatics& st = demo_statics();
  if (st.situations) return *st.situations;
  std::vector<Value> list;
  for (double n = 1; n <= 8; ++n) {
    list.push_back(sit(to_string(Value(n)) + u" Players Stage Mode", true, teams_fill(n, u"1")));
  }
  for (double n = 2; n <= 8; ++n) {
    list.push_back(
        sit(to_string(Value(n)) + u" Players, VS Mode", false, teams_range(n)));
  }
  list.push_back(sit(u"2 Teams, 4 Players, VS Mode", false,
                     cat(teams_fill(2, u"1"), teams_fill(2, u"2"))));
  list.push_back(sit(u"3 Teams, 6 Players, VS Mode", false,
                     cat(teams_fill(3, u"1"), teams_fill(3, u"2"))));
  list.push_back(sit(u"4 Teams, 8 Players, VS Mode", false,
                     cat(teams_fill(4, u"1"), teams_fill(4, u"2"))));
  list.push_back(sit(u"3 Teams, 6 Players, VS Mode", false,
                     cat(cat(teams_fill(2, u"1"), teams_fill(2, u"2")), teams_fill(2, u"3"))));
  list.push_back(sit(u"4 Teams, 8 Players, VS Mode", false,
                     cat(cat(cat(teams_fill(2, u"1"), teams_fill(2, u"2")), teams_fill(2, u"3")),
                         teams_fill(2, u"4"))));
  list.push_back(sit(
      u"Julian VS 10 Fighters", false,
      cat(teams_fill(10, u"2"), teams_fill(10, u"1")),
      oids_of({oid::kJulian, oid::kJustin, oid::kJustin, oid::kJustin, oid::kJustin, oid::kJustin,
               oid::kJustin, oid::kJustin, oid::kJustin, oid::kJustin, oid::kDeep, oid::kJohn,
               oid::kHenry, oid::kRudolf, oid::kLouis, oid::kFiren, oid::kFreeze, oid::kDennis,
               oid::kWoody, oid::kDavis})));
  list.push_back(sit(
      u" VS 8 Fighters", false,
      cat(teams_fill(8, u"2"), teams_fill(8, u"1")),
      oids_of({oid::kFirzen, oid::kJan, oid::kSorcerer, oid::kSorcerer, oid::kSorcerer,
               oid::kSorcerer, oid::kSorcerer, oid::kSorcerer, oid::kDeep, oid::kJohn,
               oid::kHenry, oid::kRudolf, oid::kLouis, oid::kDennis, oid::kWoody, oid::kDavis})));
  list.push_back(sit(
      u"LouisEX VS 9 Fighters", false,
      cat(teams_fill(7, u"2"), teams_fill(9, u"1")),
      oids_of({oid::kLouisEX, oid::kMonk, oid::kMonk, oid::kMonk, oid::kMonk, oid::kMonk,
               oid::kMonk, oid::kDeep, oid::kJohn, oid::kHenry, oid::kRudolf, oid::kDavis,
               oid::kFiren, oid::kFreeze, oid::kDennis, oid::kWoody})));
  st.situations = std::make_unique<Randoming>(u"demo_situation_randoming", std::move(list),
                                              &lfw.mt_ref());
  return *st.situations;
}

const Value& DemoModeLogic::get_situation(LFW& lfw) {
  DemoStatics& st = demo_statics();
  if (truthy(st.situation)) return st.situation;
  st.situation = get_situations(lfw).get();
  return st.situation;
}

void DemoModeLogic::clear_situation() { demo_statics().situation = Value(); }

Randoming& DemoModeLogic::get_stages(LFW& lfw) {
  DemoStatics& st = demo_statics();
  if (st.stages) return *st.stages;
  std::vector<Value> src;
  for (const Value& v : lfw.datas().stages()) {
    const Value g = field_or(v, u"group");
    if (!group_some_ne(g, stage_group::kHidden)) continue;
    if (!group_some_ne(g, stage_group::kDev)) continue;
    if (!truthy(field_or(v, u"is_starting"))) continue;
    src.push_back(v);
  }
  st.stages = std::make_unique<Randoming>(u"stage_randoming", std::move(src), &lfw.mt_ref());
  return *st.stages;
}

bool DemoModeLogic::is_stage_mode() {
  return truthy(field_or(get_situation(lfw()), u"stage_mode"));
}

bool DemoModeLogic::is_vs_mode() {
  return !truthy(field_or(get_situation(lfw()), u"stage_mode"));
}

void DemoModeLogic::handle_stage_actions(const Value& actions) {
  const Array* const arr = as_array(actions);
  if (arr == nullptr) return;
  for (const Value& action : arr->items()) {
    if (!std::holds_alternative<std::u16string>(action)) continue;
    const std::u16string& name = std::get<std::u16string>(action);
    if (name == stage_actions::kGoGoGoRight) {
      ComponentsPlayer* const p = gogogo();
      if (p != nullptr) {
        p->start();
        p->node.set_visible(true);
      }
    } else if (name == stage_actions::kLoopGoGoGoRight) {
      ComponentsPlayer* const p = gogogo_loop();
      if (p != nullptr) {
        p->start();
        p->node.set_visible(true);
      }
    }
  }
}

void DemoModeLogic::listen_stage(stage::Stage* stage) {
  if (_stage == stage) return;
  if (_stage_remover_pc) {
    _stage_remover_pc();
    _stage_remover_pc = nullptr;
  }
  if (_stage_remover_req) {
    _stage_remover_req();
    _stage_remover_req = nullptr;
  }
  _stage = stage;
  if (stage == nullptr) return;
  _stage_remover_pc = stage->callbacks().on(
      u"on_phase_changed", [this](const stage::StageCallbacks::Payloads& p) {
        for (const stage::StageCallbackArgs& a : p) handle_phase_changed(a);
      });
  _stage_remover_req = stage->callbacks().on(
      u"on_requrie_goto_next_stage", [this](const stage::StageCallbacks::Payloads& p) {
        (void)p;
        Jalousie* const j = jalousie();
        if (j != nullptr) j->set_open(false);
      });
}

void DemoModeLogic::remove_listeners() {
  if (_stage_remover_pc) {
    _stage_remover_pc();
    _stage_remover_pc = nullptr;
  }
  if (_stage_remover_req) {
    _stage_remover_req();
    _stage_remover_req = nullptr;
  }
  if (_world_remover) {
    _world_remover();
    _world_remover = nullptr;
  }
  _stage = nullptr;
}

void DemoModeLogic::handle_phase_changed(const stage::StageCallbackArgs& a) {
  stage::Stage* const stage = a.stage;
  const Value& curr = a.a;
  const Value& prev = a.b;
  if (stage == nullptr || stage->is_chapter_finish()) return;
  if (is_nullish(prev)) {
    stop_gogogo(gogogo());
    stop_gogogo(gogogo_loop());
  }
  const Value on_end = is_nullish(prev) ? Value() : field_or(prev, u"on_end");
  const Value on_start = is_nullish(curr) ? Value() : field_or(curr, u"on_start");
  const Array* const end_arr = as_array(on_end);
  if (end_arr != nullptr && !end_arr->empty()) handle_stage_actions(on_end);
  const Array* const start_arr = as_array(on_start);
  if (start_arr != nullptr && !start_arr->empty()) handle_stage_actions(on_start);
}

void DemoModeLogic::startup() {
  LFW& l = lfw();
  _fsm.use(Value(mode_state_str(ModeState::Running)));
  if (UINode* const n = focus_text_node()) n->set_visible(false);

  const bool stage_mode = is_stage_mode();
  Value stage = Value();
  if (stage_mode) {
    stage = get_stages(l).get();
    l.change_bg(stage_bg_id(stage));
  } else {
    l.mt()->mark = u"dmg_startup_bg";
    const std::vector<Value>& bgs = l.datas().backgrounds();
    Value bg_arr = Value(std::make_shared<Array>(bgs));
    const Value bg = l.mt()->pick_value(bg_arr);
    l.change_bg(bg_id_or_question(bg));
  }
  const std::vector<Value> regular = l.datas().get_fighters_of_group(u"1000");
  const std::vector<Value> boss = l.datas().get_fighters_of_group(u"Boss");
  Value fighters_arr = Value(std::make_shared<Array>(regular));
  const bool vs = is_vs_mode();

  world().set_paused(false);
  Background* const bg = world().bg();
  const double far = bg->far_plane();
  const double near_v = bg->near_plane();
  const double left = bg->left();
  const double right = bg->right();
  UIComponent* const wr_c = node.search_component(WeaponRain::class_tag());
  if (wr_c != nullptr && is_class(wr_c->clazz(), WeaponRain::class_tag())) {
    static_cast<WeaponRain*>(wr_c)->props_holder.set_str(
        u"groups", Value(std::u16string(vs ? entity_group::kStageWeapon : entity_group::kVsWeapon)));
  }
  if (vs) {
    l.sounds_play_bgm(Value(u"?"));
  } else {
    auto arr = std::get<std::shared_ptr<Array>>(fighters_arr);
    for (const Value& b : boss) arr->push_back(b);
  }

  l.mt()->mark = u"demo_startup_cam_x";
  // 可见宽度 = screen / zoom（bg 的 zoom 会缩放世界，zoom>1 时视口更窄）
  const double zx = bg->zoom_x();
  const double use_zx = (zx == 0.0 || std::isnan(zx)) ? 1.0 : zx;
  const double view_w = to_number(world().dataset.get(u"screen_w")) / use_zx;
  double cam_x = 0.0;
  if (!stage_mode) cam_x = l.mt()->range(left, right - view_w);
  const double min_x = stage_mode ? (cam_x + 40.0) : (cam_x + 1.0 * view_w / 3.0);
  const double max_x = stage_mode ? 80.0 : (cam_x + 2.0 * view_w / 3.0);

  const Value& situation = get_situation(l);
  if (UINode* const n = situation_name()) n->set_text(to_string(field_or(situation, u"title")));
  const Value teams_v = field_or(situation, u"teams");
  const Value oids_v = field_or(situation, u"oids");
  const Array* const teams = as_array(teams_v);
  const Array* const oids = as_array(oids_v);
  const size_t len = std::max(teams != nullptr ? teams->size() : 0,
                              oids != nullptr ? oids->size() : 0);
  std::vector<Entity*> fighters;
  for (size_t i = 0; i < len; ++i) {
    const bool has_team = teams != nullptr && i < teams->size();
    const bool has_oid = oids != nullptr && i < oids->size();
    const Value oid = has_oid ? oids->at(i) : Value();
    Value fighter_data = Value();
    if (truthy(oid)) {
      const Value* const f = l.datas().find(oid);
      if (f != nullptr) fighter_data = *f;
    } else {
      l.mt()->mark = u"demo_startup_fighter_" + to_string(Value(static_cast<double>(i)));
      fighter_data = l.mt()->take_value(fighters_arr);
    }
    if (!truthy(fighter_data)) continue;

    PlayerInfo* const player = l.player(to_string(Value(static_cast<double>(i))));
    Entity* const fighter = l.create_entity_with_bot(player->id(), world(), fighter_data);
    if (fighter == nullptr) continue;
    fighter->set_team(has_team ? to_string(teams->at(i)) : l.new_team());
    l.mt()->mark = u"demo_startup_fighter_facing";
    if (stage_mode) {
      fighter->facing = static_cast<double>(FacingFlag::Right);
    } else {
      const std::optional<double> pick = l.mt()->pick(std::vector<double>{
          static_cast<double>(FacingFlag::Left), static_cast<double>(FacingFlag::Right)});
      fighter->facing = pick.value();
    }
    fighter->as_key_role(Value(true));

    l.mt()->mark = u"demo_startup_fighter_x";
    const double x = l.mt()->range(min_x, max_x);
    l.mt()->mark = u"demo_startup_fighter_z";
    const double z = l.mt()->range(far, near_v);
    fighter->set_position(Value(x), Value(), Value(z));
    fighter->set_blinking(to_number(world().dataset.get(u"begin_blink_time")));
    fighter->set_invulnerable(to_number(world().dataset.get(u"begin_blink_time")));
    if (vs) fighter->set_mp(fighter->mp_max() * 2.0 / 5.0);
    fighter->attach();
    fighters.push_back(fighter);
  }

  std::vector<FighterStatBar*> stat_bars;
  for (UIComponent* const c : node.search_components(FighterStatBar::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), FighterStatBar::class_tag())) {
      stat_bars.push_back(static_cast<FighterStatBar*>(c));
    }
  }
  for (size_t i = 0; i < stat_bars.size();) {
    FighterStatBar* const stat_bar = stat_bars[i];
    Entity* const fighter = i < fighters.size() ? fighters[i] : nullptr;
    stat_bar->node.set_visible(fighter != nullptr);
    stat_bar->node.set_disabled(fighter == nullptr);
    stat_bar->set_entity(fighter);
    if (fighter != nullptr) {
      ++i;
      continue;
    }
    stat_bars.erase(stat_bars.begin() + static_cast<std::ptrdiff_t>(i));
  }

  if (stage_mode && !is_nullish(stage)) {
    l.change_stage(stage_id_or_empty(stage));
    listen_stage(world().stage());
  }
  if (!_world_remover) {
    _world_remover = world().callbacks.on(
        u"on_stage_change", [this](const WorldCallbacks::Payloads& p) {
          for (const WorldCallbackArgs& a : p) {
            (void)a;
            if (!is_stage_mode()) return;
            listen_stage(a.stage);
            stop_gogogo(gogogo());
            stop_gogogo(gogogo_loop());
            Jalousie* const j = jalousie();
            if (j != nullptr) j->set_open(true);
          }
        });
  }
  CameraCtrl* const cam = cam_ctrl();
  if (cam != nullptr) cam->set_staring(fighters.empty() ? nullptr : fighters[0]);
  world().camera().jump_x(cam_x);
}

void DemoModeLogic::clearup() {
  remove_listeners();
  world().clear();
  clear_situation();
}

void DemoModeLogic::on_start() {
  UIComponent::on_start();
  if (lfw().first_page != u"init_demo") {
    UINode* const link = node.search_node(u"demo_play_link");
    if (link != nullptr) link->set_visible(false);
  }
  startup();
}

void DemoModeLogic::on_stop() {
  UIComponent::on_stop();
  clearup();
}

void DemoModeLogic::update(double dt) {
  CameraCtrl* const cam = cam_ctrl();
  if (cam != nullptr) {
    Entity* const staring = cam->staring();
    const std::optional<Entity*> staring_opt =
        staring != nullptr ? std::optional<Entity*>(staring) : std::nullopt;
    const std::optional<bool> free_opt = cam->auto_;
    if (_staring != staring_opt || _free != free_opt) {
      _staring = staring_opt;
      _free = free_opt;
      if (!cam->auto_) {
        if (UINode* const n = focus_prefix()) n->set_text(u"cam_controlling");
        if (UINode* const n = focus_on()) n->set_visible(false);
      }
      const std::u16string txt =
          staring != nullptr ? (u"[" + staring->team() + u"] " + to_string(staring->name())) : u"-";
      if (UINode* const n = focus_prefix()) n->set_text(u"curr_focus");
      if (UINode* const n = focus_on()) {
        n->set_visible(true);
        n->set_text(txt);
      }
    }
  }
  if (is_stage_mode()) {
    Jalousie* const j = jalousie();
    if (j != nullptr && !j->open() && j->anim().done()) {
      lfw().goto_next_stage();
      _fsm.use(Value(mode_state_str(ModeState::Running)));
      j->set_open(true);
    }
  }
  _fsm.update(dt);
}

void DemoModeLogic::on_key_down(LFWKeyEvent& e) {
  if (e.game_key == gk::ka) {
    clearup();
    startup();
  }
}

}
}
