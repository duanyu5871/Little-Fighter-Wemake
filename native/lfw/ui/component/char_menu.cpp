#include "lfw/ui/component/char_menu.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "lfw/core/js_string.h"
#include "lfw/defines/cheat_type.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/team_enum.h"
#include "lfw/lfw.h"
#include "lfw/player_info.h"
#include "lfw/ui/component/char_menu_head.h"
#include "lfw/ui/component/fighter_name.h"
#include "lfw/ui/component/game_prepare_logic.h"
#include "lfw/ui/component/player_name.h"
#include "lfw/ui/component/player_team_name.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {
namespace ui {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

bool has_prefix(const std::u16string& s, const char16_t* prefix) {
  const std::u16string p(prefix);
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

std::optional<double> prop_num(UIProps& holder, const char16_t* key) {
  const Value v = holder.prop_value(std::u16string(key));
  const double* const n = std::get_if<double>(&v);
  return n != nullptr ? std::optional<double>(*n) : std::nullopt;
}

bool prop_bool(UIProps& holder, const char16_t* key) {
  const Value v = holder.prop_value(std::u16string(key));
  const bool* const b = std::get_if<bool>(&v);
  return b != nullptr ? *b : false;
}

// `string[] | undefined`（非数组/非字符串项一律跳过）。
std::vector<std::u16string> strs_of(const Value& v) {
  std::vector<std::u16string> out;
  const Array* const a = as_array(v);
  if (a == nullptr) return out;
  for (size_t i = 0; i < a->size(); ++i) {
    if (const std::u16string* const s = std::get_if<std::u16string>(&a->at(i))) out.push_back(*s);
  }
  return out;
}

bool same_object(const Value& a, const Value& b) {
  const Object* const oa = as_object(a);
  return oa != nullptr && oa == as_object(b);
}

std::shared_ptr<Object> num_prop() {
  auto p = std::make_shared<Object>();
  p->set(u"type", Value(u"number"));
  p->set(u"nullable", Value(true));
  return p;
}

std::shared_ptr<Object> bool_prop() {
  auto p = std::make_shared<Object>();
  p->set(u"type", Value(u"boolean"));
  p->set(u"nullable", Value(true));
  return p;
}

std::shared_ptr<Object> str_arr_prop() {
  auto p = std::make_shared<Object>();
  p->set(u"type", Value(u"array"));
  auto items = std::make_shared<Object>();
  items->set(u"type", Value(u"string"));
  p->set(u"items", Value(items));
  p->set(u"nullable", Value(true));
  return p;
}

// `Defines.Teams.map(v => v.toString())`。
std::vector<std::u16string> default_teams() {
  std::vector<std::u16string> out;
  const Value* const v = defines::find(u"Defines.Teams");
  const Array* const arr = v != nullptr ? as_array(*v) : nullptr;
  if (arr == nullptr) return out;
  for (size_t i = 0; i < arr->size(); ++i) out.push_back(to_string(arr->at(i)));
  return out;
}

// `Defines.TeamInfoMap[team]`。
const Object* team_info(const std::u16string& team) {
  const Value* const map = defines::find(u"Defines.TeamInfoMap");
  const Object* const obj = map != nullptr ? as_object(*map) : nullptr;
  if (obj == nullptr) return nullptr;
  const Value* const info = obj->get(team);
  return info != nullptr ? as_object(*info) : nullptr;
}

}

std::u16string char_menu_state_str(CharMenuState s) {
  switch (s) {
    case CharMenuState::PlayerSel: return u"Player";
    case CharMenuState::CountingDown: return u"CountingDown";
    case CharMenuState::ComNumSel: return u"ComNumberSel";
    case CharMenuState::ComSel: return u"Computer";
    case CharMenuState::GameSetting: return u"GameSetting";
  }
  return std::u16string();
}

CharMenuState_Base::CharMenuState_Base(CharMenuState key, CharMenuLogic& owner)
    : state_key(key), owner(owner), lfw(owner.lfw()) {
  name = char_menu_state_str(key);
}

Value CharMenuState_Base::key() const { return Value(char_menu_state_str(state_key)); }

CharMenuState_PlayerSel::CharMenuState_PlayerSel(CharMenuLogic& owner)
    : CharMenuState_Base(CharMenuState::PlayerSel, owner) {}

void CharMenuState_PlayerSel::on_key_down(LFWKeyEvent& e) {
  PlayerInfo* const player = lfw.players_get(e.player);
  if (player == nullptr) return;
  if (e.game_key == gk::kL) owner.press_lr(player, -1.0);
  else if (e.game_key == gk::kR) owner.press_lr(player, 1.0);
  else if (e.game_key == gk::ka) owner.press_a(player);
  else if (e.game_key == gk::kj) owner.press_j(player);
  else if (e.game_key == gk::kU) owner.press_u(player);
}

void CharMenuState_PlayerSel::enter() {
  std::vector<PlayerInfo*> coms;
  for (const std::pair<PlayerInfo*, SlotState>& kv : owner.players) {
    if (kv.first->is_com()) coms.push_back(kv.first);
  }
  for (PlayerInfo* const com : coms) owner.map_erase(owner.players, com);
  owner.update_slots();
}

std::optional<Value> CharMenuState_PlayerSel::update(double dt) {
  (void)dt;
  // 生存排行（无设置菜单、人数固定）：人数不够时不进入倒计时
  UIComponent* const c = owner.node.root().search_component(GamePrepareLogic::class_tag());
  GamePrepareLogic* const gpl =
      c != nullptr && is_class(c->clazz(), GamePrepareLogic::class_tag())
          ? static_cast<GamePrepareLogic*>(c)
          : nullptr;
  const double need =
      gpl != nullptr && gpl->auto_start_when_ready() ? owner.min_player() : 1.0;
  bool all_ready = static_cast<double>(owner.players.size()) >= need;
  for (const std::pair<PlayerInfo*, SlotState>& kv : owner.players) {
    if (kv.second.step == SlotStep::Ready) continue;
    all_ready = false;
    break;
  }
  if (all_ready) return Value(char_menu_state_str(CharMenuState::CountingDown));
  return std::nullopt;
}

CharMenuState_CountingDown::CharMenuState_CountingDown(CharMenuLogic& owner)
    : CharMenuState_Base(CharMenuState::CountingDown, owner) {}

void CharMenuState_CountingDown::enter() { _count_down = 5000.0; }

void CharMenuState_CountingDown::on_key_down(LFWKeyEvent& e) {
  PlayerInfo* const player = lfw.players_get(e.player);
  // 生存排行：按“防御”取消倒计时，退回选角
  UIComponent* const c = owner.node.root().search_component(GamePrepareLogic::class_tag());
  GamePrepareLogic* const gpl =
      c != nullptr && is_class(c->clazz(), GamePrepareLogic::class_tag())
          ? static_cast<GamePrepareLogic*>(c)
          : nullptr;
  const bool survival = gpl != nullptr && gpl->auto_start_when_ready();
  if (survival && e.game_key == gk::kd && player != nullptr && owner.map_has(player)) {
    owner.cancel_counting_down(player);
    return;
  }
  _count_down = max(0.0, _count_down - 500.0);
  if (owner.max_player() <= static_cast<double>(owner.players.size())) return;
  if (player == nullptr || e.game_key != gk::ka || owner.map_has(player)) return;
  owner.press_a(player);
  owner.fsm_ref().use(Value(char_menu_state_str(CharMenuState::PlayerSel)));
}

std::optional<Value> CharMenuState_CountingDown::update(double dt) {
  _count_down = max(0.0, _count_down - dt);
  const double num = ceil(_count_down / 1000.0);
  for (ISlotPack& v : owner.slots) {
    if (v.head != nullptr) v.head->count_down(num);
  }
  if (num > 0) return std::nullopt;
  // 生存排行（无设置菜单）：就绪后直接开始，并回到选角态（避免返回时自动连开 ）
  UIComponent* const c = owner.node.root().search_component(GamePrepareLogic::class_tag());
  GamePrepareLogic* const gpl =
      c != nullptr && is_class(c->clazz(), GamePrepareLogic::class_tag())
          ? static_cast<GamePrepareLogic*>(c)
          : nullptr;
  if (gpl != nullptr && gpl->auto_start_when_ready() &&
      owner.max_player() <= static_cast<double>(owner.players.size())) {
    lfw.broadcast(Value(u"start_game"));
    owner.reset();
    return std::nullopt;
  }
  return Value(char_menu_state_str(owner.max_player() <= static_cast<double>(owner.players.size())
                                       ? CharMenuState::GameSetting
                                       : CharMenuState::ComNumSel));
}

void CharMenuState_CountingDown::leave() {
  for (ISlotPack& v : owner.slots) {
    if (v.head != nullptr) v.head->count_down(0);
  }
}

CharMenuState_ComNumSel::CharMenuState_ComNumSel(CharMenuLogic& owner)
    : CharMenuState_Base(CharMenuState::ComNumSel, owner) {}

void CharMenuState_ComNumSel::enter() {how_many_coms = owner.node.root().search_node(u"how_many_computer");
  if (how_many_coms != nullptr) how_many_coms->set_visible(true);
  btns.clear();
  for (int i = 0; i < 8; ++i) {
    btns.push_back(how_many_coms != nullptr
                       ? how_many_coms->search_node(u"com_num_" +
                                                    std::u16string(1, static_cast<char16_t>(u'0' + i)))
                       : nullptr);
  }
  const double min_coms = owner.min_coms();
  const double max_coms = owner.max_coms();
  for (size_t i = 0; i < btns.size(); ++i) {
    UINode* const btn = btns[i];
    if (btn == nullptr) continue;
    btn->set_disabled(!(static_cast<double>(i) >= min_coms && static_cast<double>(i) <= max_coms));
    if (static_cast<double>(i) == min_coms) btn->set_focused(true);
  }
  double num = 0;
  for (const std::pair<PlayerInfo*, SlotState>& kv : owner.prev_players) {
    if (kv.first->is_com()) ++num;
  }
  num = clamp(num, min_coms, max_coms);
  if (btns.size() > static_cast<size_t>(num) && btns[static_cast<size_t>(num)] != nullptr) {
    btns[static_cast<size_t>(num)]->set_focused(true);
  }
  com_num = -1.0;
}

std::optional<Value> CharMenuState_ComNumSel::update(double dt) {
  (void)dt;
  if (how_many_coms == nullptr || !how_many_coms->visible()) {
    return Value(char_menu_state_str(CharMenuState::GameSetting));
  }
  if (com_num > 0) owner.fsm_ref().use(Value(char_menu_state_str(CharMenuState::ComSel)));
  if (com_num == 0) owner.fsm_ref().use(Value(char_menu_state_str(CharMenuState::GameSetting)));
  return std::nullopt;
}

void CharMenuState_ComNumSel::leave() {
  if (how_many_coms != nullptr) how_many_coms->set_visible(false);
  com_num = -1.0;
}

void CharMenuState_ComNumSel::on_key_down(LFWKeyEvent& e) {
  if (e.game_key != gk::ka) return;
  UINode* const focused = owner.node.root().focused_node();
  std::optional<double> which;
  if (focused != nullptr) {
    const std::u16string id = to_string(focused->id());
    const size_t at = id.find(u"com_num_");
    if (at != std::u16string::npos && at + 8 < id.size()) {
      const char16_t c = id[at + 8];
      if (c >= u'0' && c <= u'9') which = static_cast<double>(c - u'0');
    }
  }
  // TS `Number(undefined)` = NaN。
  com_num = which.has_value() ? *which : std::numeric_limits<double>::quiet_NaN();
  owner.com_num = com_num;
}

bool CharMenuState_ComSel::all_com_ready() {
  double count = 0;
  for (const std::pair<PlayerInfo*, SlotState>& kv : owner.players) {
    if (!kv.first->is_com()) continue;
    if (kv.second.step != SlotStep::Ready) continue;
    ++count;
  }
  return count == owner.com_num;
}

CharMenuState_ComSel::CharMenuState_ComSel(CharMenuLogic& owner)
    : CharMenuState_Base(CharMenuState::ComSel, owner) {}

void CharMenuState_ComSel::enter() { owner.add_com(); }

void CharMenuState_ComSel::on_key_down(LFWKeyEvent& e) {
  if (e.game_key == gk::ka) {
    std::optional<std::pair<PlayerInfo*, SlotState*>> pair = owner.last_com();
    if (!pair.has_value()) {
      owner.add_com();
      return;
    }
    PlayerInfo* const player = pair->first;
    if (!player->is_com()) {
      owner.add_com();
      return;
    }
    owner.press_a(player);
    SlotState* const state = owner.find_state(owner.players, player);
    if (state == nullptr || state->step != SlotStep::Ready) return;
    if (all_com_ready()) return;
    owner.add_com();
    return;
  }

  const std::optional<std::pair<PlayerInfo*, SlotState*>> pair = owner.last_com();
  if (!pair.has_value()) return;
  PlayerInfo* const com = pair->first;
  if (e.game_key == gk::kj) owner.press_j(com);
  else if (e.game_key == gk::kL) owner.press_lr(com, -1.0);
  else if (e.game_key == gk::kR) owner.press_lr(com, 1.0);
  else if (e.game_key == gk::kU) owner.press_u(com);
  else return;
  if (!owner.map_has(com)) {
    lfw.players_delete(com->id());
    const std::optional<std::pair<PlayerInfo*, SlotState*>> lc = owner.last_com();
    if (lc.has_value()) owner.press_j(lc->first);
  }
}

std::optional<Value> CharMenuState_ComSel::update(double dt) {
  (void)dt;
  const std::optional<std::pair<PlayerInfo*, SlotState*>> pair = owner.last_com();
  if (!pair.has_value()) return Value(char_menu_state_str(CharMenuState::ComNumSel));
  if (all_com_ready()) return Value(char_menu_state_str(CharMenuState::GameSetting));
  return std::nullopt;
}

CharMenuState_GameSetting::CharMenuState_GameSetting(CharMenuLogic& owner)
    : CharMenuState_Base(CharMenuState::GameSetting, owner) {}

void CharMenuState_GameSetting::enter() {
  owner.update_slots();
  game_settings_menu = owner.node.root().search_node(u"game_settings_menu");
  if (game_settings_menu != nullptr) game_settings_menu->set_visible(true);
}

void CharMenuState_GameSetting::leave() {
  if (game_settings_menu != nullptr) game_settings_menu->set_visible(false);
  owner.update_random();
}

const ClazzTag* CharMenuLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& CharMenuLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"CharMenuLogic"};
  return tags;
}

const std::u16string& CharMenuLogic::props_tag() const {
  static const std::u16string tag = u"CharMenuLogic";
  return tag;
}

const Value& CharMenuLogic::PROPS() {
  static const Value props = [] {
    auto o = std::make_shared<Object>();
    o->set(u"min_player", Value(num_prop()));
    o->set(u"max_player", Value(num_prop()));
    o->set(u"teams", Value(str_arr_prop()));
    o->set(u"oids", Value(str_arr_prop()));
    o->set(u"auto_join", Value(bool_prop()));
    return Value(o);
  }();
  return props;
}

CharMenuLogic::CharMenuLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info), _fsm(u"CharMenuFSM") {
  _states.push_back(std::make_unique<CharMenuState_PlayerSel>(*this));
  _states.push_back(std::make_unique<CharMenuState_CountingDown>(*this));
  _states.push_back(std::make_unique<CharMenuState_ComNumSel>(*this));
  _states.push_back(std::make_unique<CharMenuState_ComSel>(*this));
  _states.push_back(std::make_unique<CharMenuState_GameSetting>(*this));
  _fsm.add(_states[0].get(), _states[1].get(), _states[2].get(), _states[3].get(),
           _states[4].get());
  _fsm.use(Value(char_menu_state_str(CharMenuState::PlayerSel)));
}

CharMenuLogic::~CharMenuLogic() { unsubscribe_lfw(); }

double CharMenuLogic::max_player() { return prop_num(props_holder, u"max_player").value_or(8.0); }

void CharMenuLogic::set_max_player(double v) {
  props_holder.set_num(u"max_player", Value(v));
}

double CharMenuLogic::min_player() { return prop_num(props_holder, u"min_player").value_or(8.0); }

void CharMenuLogic::set_min_player(double v) {
  props_holder.set_num(u"min_player", Value(v));
}

double CharMenuLogic::max_coms() {
  return max(0.0, max_player() - static_cast<double>(players.size()));
}

double CharMenuLogic::min_coms() {
  return max(0.0, min_player() - static_cast<double>(players.size()));
}

std::vector<std::u16string> CharMenuLogic::teams() {
  const Value v = props_holder.prop_value(u"teams");
  const Array* const arr = as_array(v);
  if (arr == nullptr) return default_teams();
  std::vector<std::u16string> out;
  for (size_t i = 0; i < arr->size(); ++i) out.push_back(to_string(arr->at(i)));
  return out;
}

void CharMenuLogic::set_teams(std::vector<std::u16string> v) {
  props_holder.set_strs(u"teams", std::move(v));
}

std::optional<std::vector<std::u16string>> CharMenuLogic::oids() {
  const Value v = props_holder.prop_value(u"oids");
  if (is_nullish(v)) return std::nullopt;
  return strs_of(v);
}

void CharMenuLogic::set_oids(const std::optional<std::vector<std::u16string>>& v) {
  if (!v.has_value()) {
    props_holder.set_str(u"oids", Value());
  } else {
    props_holder.set_strs(u"oids", *v);
  }
  // 限制名单变化后重建随机池，保证“随机”也从限制名单内挑
  if (_randoming_ready) {
    rebuild_randoming();
    handle_fighters_changed();
  }
}

std::vector<Value> CharMenuLogic::filter_oids(const std::vector<Value>& list) {
  const std::optional<std::vector<std::u16string>> allowed = oids();
  if (!allowed.has_value() || allowed->empty()) return list;
  std::vector<Value> ret;
  for (const Value& v : list) {
    const Value* const id = field_of(v, u"id");
    const std::u16string s = id != nullptr ? to_string(*id) : std::u16string();
    for (const std::u16string& o : *allowed) {
      if (o == s) {
        ret.push_back(v);
        break;
      }
    }
  }
  return ret;
}

void CharMenuLogic::rebuild_randoming() {
  const std::vector<Value> fighters_v =
      filter_oids(lfw().datas().get_fighters_of_group(u"1000"));
  _randoming = std::make_unique<Randoming>(u"charmenu_fighter_randoming", fighters_v,
                                           &lfw().mt_ref());
  _randoming_ready = true;
  _randoming_fighters = lfw().datas().fighters().size();
}

void CharMenuLogic::ensure_randoming() {
  if (_randoming_fighters == lfw().datas().fighters().size()) return;
  rebuild_randoming();
  for (std::pair<PlayerInfo*, SlotState>& kv : players) {
    if (!truthy(kv.second.fighter)) kv.second.fighter = randoming_next();
  }
  update_slots();
}

Value CharMenuLogic::randoming_next() {
  if (!_randoming) return Value(NullTag{});
  const Value v = _randoming->get();
  return is_nullish(v) ? Value(NullTag{}) : v;
}

bool CharMenuLogic::group_has(const Value& data, const char16_t* group) {
  const Value* const base = field_of(data, u"base");
  const Value* const gv = base != nullptr ? field_of(*base, u"group") : nullptr;
  const Array* const arr = gv != nullptr ? as_array(*gv) : nullptr;
  if (arr == nullptr) return false;
  for (size_t i = 0; i < arr->size(); ++i) {
    if (equals(arr->at(i), Value(std::u16string(group)))) return true;
  }
  return false;
}

std::vector<Value> CharMenuLogic::fighters() {
  const bool cheat_0 = lfw().is_cheat(u"LF2_NET");
  const bool cheat_1 = lfw().is_cheat(u"GIM_INK");
  const std::vector<Value>& all = lfw().datas().fighters();
  if (cheat_0 && cheat_1) return filter_oids(all);
  std::vector<Value> ret;
  for (const Value& v : all) {
    if (!cheat_0 && group_has(v, entity_group::kHidden)) continue;
    if (!cheat_1 && group_has(v, entity_group::kDev)) continue;
    ret.push_back(v);
  }
  return filter_oids(ret.empty() ? all : ret);
}

void CharMenuLogic::subscribe_lfw() {
  if (!_removers.empty()) return;
  _removers.push_back(
      lfw().callbacks.on(u"on_loading_end", [this](const LfwCallbacks::Payloads&) {
        ensure_randoming();
      }));
  _removers.push_back(
      lfw().callbacks.on(u"on_cheat_changed", [this](const LfwCallbacks::Payloads& p) {
        const std::u16string cheat_name = p.empty() ? std::u16string() : p[0].text;
        const bool enabled = !p.empty() && p[0].flag;
        if (cheat_name == u"LF2_NET" && !enabled) handle_fighters_changed();
      }));
  _removers.push_back(
      lfw().callbacks.on(u"on_broadcast", [this](const LfwCallbacks::Payloads& p) {
        const std::u16string message = p.empty() ? std::u16string() : p[0].text;
        if (message == u"reset_gpl") {
          reset();
          return;
        }
        if (message == u"update_random") {
          update_random();
          return;
        }
        // UI 左/右按钮：等价键盘 L/R 切换角色
        if (message == u"char_menu_pick_prev") {
          pick_lr(-1.0);
          return;
        }
        if (message == u"char_menu_pick_next") {
          pick_lr(1.0);
          return;
        }
        if (has_prefix(message, u"char_menu_pick_prev_")) {
          const double idx = to_number(Value(message.substr(20)));
          pick_lr(-1.0, idx - 1.0);
          return;
        }
        if (has_prefix(message, u"char_menu_pick_next_")) {
          const double idx = to_number(Value(message.substr(20)));
          pick_lr(1.0, idx - 1.0);
          return;
        }
      }));
}

void CharMenuLogic::unsubscribe_lfw() {
  for (std::function<void()>& r : _removers) {
    if (r) r();
  }
  _removers.clear();
}

void CharMenuLogic::on_start() {
  UIComponent::on_start();
  rebuild_randoming();
  subscribe_lfw();
  std::vector<CharMenuHead*> heads;
  for (UIComponent* const c : node.search_components(CharMenuHead::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), CharMenuHead::class_tag())) {
      heads.push_back(static_cast<CharMenuHead*>(c));
    }
  }
  std::vector<PlayerName*> p_nam;
  for (UIComponent* const c : node.search_components(PlayerName::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), PlayerName::class_tag())) {
      p_nam.push_back(static_cast<PlayerName*>(c));
    }
  }
  std::vector<FighterName*> f_nam;
  for (UIComponent* const c : node.search_components(FighterName::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), FighterName::class_tag())) {
      f_nam.push_back(static_cast<FighterName*>(c));
    }
  }
  std::vector<PlayerTeamName*> t_nam;
  for (UIComponent* const c : node.search_components(PlayerTeamName::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), PlayerTeamName::class_tag())) {
      t_nam.push_back(static_cast<PlayerTeamName*>(c));
    }
  }
  const size_t len = std::max({heads.size(), p_nam.size(), f_nam.size(), t_nam.size()});
  for (size_t i = 0; i < len; ++i) {
    ISlotPack e;
    if (i < heads.size()) e.head = heads[i];
    if (i < p_nam.size()) e.player_name = p_nam[i];
    if (i < f_nam.size()) e.fighter_name = f_nam[i];
    if (i < t_nam.size()) e.team_name = t_nam[i];
    slots.push_back(e);
  }
  if (prop_bool(props_holder, u"auto_join")) do_auto_join();
}

void CharMenuLogic::do_auto_join() {
  for (const std::pair<PlayerInfo*, SlotState>& kv : players) {
    if (!kv.first->is_com()) return;
  }
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.second->is_com()) continue;
    press_a(kv.second.get());
    return;
  }
}

void CharMenuLogic::on_resume() {
  UIComponent::on_resume();
  if (prop_bool(props_holder, u"auto_join")) do_auto_join();
}

void CharMenuLogic::on_stop() {
  UIComponent::on_stop();
  unsubscribe_lfw();
}

void CharMenuLogic::on_key_down(LFWKeyEvent& e) {
  IState* const s = _fsm.state();
  if (s != nullptr) static_cast<CharMenuState_Base*>(s)->on_key_down(e);
}

void CharMenuLogic::reset() {
  prev_players.clear();
  for (const std::pair<PlayerInfo*, SlotState>& kv : players) prev_players.push_back(kv);
  players.clear();
  update_slots();
  _fsm.use(Value(char_menu_state_str(CharMenuState::PlayerSel)));
}

void CharMenuLogic::update_random() {
  ensure_randoming();
  for (std::pair<PlayerInfo*, SlotState>& kv : players) {
    if (!kv.second.random) continue;
    kv.second.fighter = randoming_next();
  }
  update_slots();
}

void CharMenuLogic::cancel_counting_down(PlayerInfo* player) {
  SlotState* const state = find_state(players, player);
  if (state == nullptr) return;
  state->step = SlotStep::FighterSel;
  lfw().host().sounds_play_preset(Value(u"cancel"), Value(), Value(), Value());
  _fsm.use(Value(char_menu_state_str(CharMenuState::PlayerSel)));
}

void CharMenuLogic::update_slots() {
  const bool random_confirm =
      _fsm.state() != nullptr &&
      strict_equals(_fsm.state()->key(), Value(char_menu_state_str(CharMenuState::GameSetting)));
  for (size_t i = 0; i < slots.size(); ++i) {
    ISlotPack& s = slots[i];
    if (i >= players.size()) {
      if (s.head != nullptr) s.head->quit();
      if (s.player_name != nullptr) s.player_name->quit();
      if (s.fighter_name != nullptr) s.fighter_name->quit();
      if (s.team_name != nullptr) s.team_name->quit();
      continue;
    }
    PlayerInfo* const player = players[i].first;
    SlotState& state = players[i].second;
    const bool is_com = player->is_com();
    if (s.head != nullptr) {
      if (state.random && !random_confirm) {
        s.head->join(std::u16string(built_in_imgs::kRFACE));
      } else if (truthy(state.fighter)) {
        const Value* const base = field_of(state.fighter, u"base");
        const Value* const head = base != nullptr ? field_of(*base, u"head") : nullptr;
        s.head->join(head != nullptr && !is_nullish(*head)
                         ? to_string(*head)
                         : std::u16string(built_in_imgs::kRFACE));
      }
    }
    if (s.player_name != nullptr) {
      s.player_name->join(to_string(player->name()), is_com, true);
    }
    if (s.fighter_name != nullptr) {
      const bool decided = state.step > SlotStep::FighterSel;
      if (state.random && !random_confirm) {
        s.fighter_name->join(to_string(lfw().string(Value(u"Random"))), is_com, decided);
      } else if (truthy(state.fighter)) {
        const Value* const base = field_of(state.fighter, u"base");
        const Value* const name_v = base != nullptr ? field_of(*base, u"name") : nullptr;
        const Value txt = lfw().string(name_v != nullptr ? *name_v : Value());
        s.fighter_name->join(truthy(txt) ? to_string(txt) : std::u16string(u"noname"), is_com,
                             decided);
      }
    }
    if (s.team_name != nullptr) {
      if (state.step >= SlotStep::TeamSel) {
        const Object* const info = team_info(state.team);
        const Value* const i18n = info != nullptr ? info->get(u"i18n") : nullptr;
        const std::u16string key =
            i18n != nullptr && !is_nullish(*i18n) ? to_string(*i18n) : state.team;
        s.team_name->join(to_string(lfw().string(Value(key))), is_com,
                          state.step > SlotStep::TeamSel);
      } else {
        s.team_name->quit();
      }
    }
  }
}

void CharMenuLogic::press_a(PlayerInfo* player) {
  ensure_randoming();
  SlotState* state = find_state(players, player);
  if (state == nullptr && max_player() <= static_cast<double>(players.size())) return;
  if (state == nullptr) {
    SlotState slot_state;
    SlotState* const prev = find_state(prev_players, player);
    if (prev != nullptr) {
      slot_state = *prev;
      map_erase(prev_players, player);
    } else {
      const std::vector<std::u16string> ts = teams();
      slot_state.team = !ts.empty() ? ts[0] : std::u16string(team_enum::kIndependent);
      slot_state.fighter = randoming_next();
    }
    slot_state.step = SlotStep::FighterSel;
    players.emplace_back(player, slot_state);
  } else if (state->step < SlotStep::Ready) {
    if (state->step == SlotStep::FighterSel && teams().size() <= 1) {
      state->step = SlotStep::Ready;
    } else {
      const double next = std::min(static_cast<double>(state->step) + 1.0,
                                   static_cast<double>(SlotStep::Ready));
      state->step = static_cast<SlotStep>(static_cast<int>(next));
    }
  } else {
    return;
  }
  lfw().host().sounds_play_preset(Value(u"join"), Value(), Value(), Value());
  update_slots();
}

bool CharMenuLogic::is_player_sel() {
  return _fsm.state() != nullptr &&
         strict_equals(_fsm.state()->key(), Value(char_menu_state_str(CharMenuState::PlayerSel)));
}

void CharMenuLogic::press_j(PlayerInfo* player) {
  SlotState* const state = find_state(players, player);
  if (state == nullptr) {
    if (is_player_sel()) {
      UIPopPageOpts opts;
      opts.min_pages = 1.0;
      node.pop_page(opts);
    }
  } else if (state->step <= SlotStep::FighterSel) {
    // auto_join：不取消加入（避免退回“按攻击加入”的未加入态），按“跳跃”直接返回上一页
    if (prop_bool(props_holder, u"auto_join")) {
      lfw().host().sounds_play_preset(Value(u"cancel"), Value(), Value(), Value());
      UIPopPageOpts opts;
      opts.min_pages = 1.0;
      node.pop_page(opts);
      return;
    }
    map_erase(players, player);
  } else {
    const double next = std::max(static_cast<double>(state->step) - 1.0,
                                 static_cast<double>(SlotStep::FighterSel));
    state->step = static_cast<SlotStep>(static_cast<int>(next));
  }
  lfw().host().sounds_play_preset(Value(u"cancel"), Value(), Value(), Value());
  update_slots();
}

void CharMenuLogic::press_lr(PlayerInfo* player, double dir) {
  SlotState* const state = find_state(players, player);
  if (state == nullptr) return;
  if (state->step == SlotStep::FighterSel) {
    const std::vector<Value> list = fighters();
    if (list.empty()) return;
    if (state->random == true || !truthy(state->fighter)) {
      state->random = false;
      state->fighter = dir < 0 ? list[list.size() - 1] : list[0];
    } else {
      double idx = -1;
      for (size_t i = 0; i < list.size(); ++i) {
        if (same_object(list[i], state->fighter)) {
          idx = static_cast<double>(i);
          break;
        }
      }
      const double next_idx = idx + dir;
      if (next_idx >= 0 && next_idx <= static_cast<double>(list.size()) - 1) {
        state->random = false;
        state->fighter = list[static_cast<size_t>(next_idx)];
      } else {
        state->random = true;
        state->fighter = randoming_next();
      }
    }
  } else if (state->step == SlotStep::TeamSel) {
    const std::vector<std::u16string> ts = teams();
    const double len = static_cast<double>(ts.size());
    double idx = -1;
    for (size_t i = 0; i < ts.size(); ++i) {
      if (ts[i] == state->team) {
        idx = static_cast<double>(i);
        break;
      }
    }
    const double next_idx = std::fmod(len + idx + dir, len);
    state->team = ts[static_cast<size_t>(next_idx)];
  }
  update_slots();
}

void CharMenuLogic::pick_lr(double dir, std::optional<double> index) {
  if (!is_player_sel()) return;
  PlayerInfo* player = nullptr;
  if (!index.has_value()) {
    for (const std::pair<PlayerInfo*, SlotState>& kv : players) {
      if (kv.first->is_com()) continue;
      player = kv.first;
      break;
    }
    if (player == nullptr) {
      for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
        if (kv.second->is_com()) continue;
        player = kv.second.get();
        break;
      }
      if (player == nullptr) return;
      press_a(player);
    }
  } else {
    if (static_cast<size_t>(*index) < players.size()) {
      player = players[static_cast<size_t>(*index)].first;
    }
    if (player == nullptr) {
      size_t at = 0;
      for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
        if (kv.second->is_com()) continue;
        if (static_cast<double>(at) == *index) {
          player = kv.second.get();
          break;
        }
        ++at;
      }
      if (player == nullptr) return;
      press_a(player);
    }
  }
  press_lr(player, dir);
}

void CharMenuLogic::press_u(PlayerInfo* player) {
  SlotState* const state = find_state(players, player);
  if (state == nullptr || state->step != SlotStep::FighterSel) return;
  state->random = true;
  state->fighter = randoming_next();
  update_slots();
}

void CharMenuLogic::handle_fighters_changed() {
  const std::vector<Value> list = fighters();
  for (std::pair<PlayerInfo*, SlotState>& kv : players) {
    bool hidden = true;
    for (const Value& v : list) {
      if (same_object(v, kv.second.fighter)) {
        hidden = false;
        break;
      }
    }
    if (!hidden) continue;
    kv.second.random = true;
    kv.second.fighter = Value(NullTag{});
  }
  update_slots();
}

void CharMenuLogic::update(double dt) {
  _fsm.update(dt);
  std::u16string key;
  for (const std::pair<PlayerInfo*, SlotState>& kv : players) {
    key += kv.first->id();
    key += u'\0';
    key += to_string(kv.first->name());
    key += u'\n';
  }
  if (key == _player_names_key) return;
  _player_names_key = key;
  update_slots();
}

std::optional<std::pair<PlayerInfo*, SlotState*>> CharMenuLogic::last_player() {
  if (players.empty()) return std::nullopt;
  std::pair<PlayerInfo*, SlotState>& kv = players.back();
  return std::make_optional(std::make_pair(kv.first, &kv.second));
}

std::optional<std::pair<PlayerInfo*, SlotState*>> CharMenuLogic::last_com() {
  const std::optional<std::pair<PlayerInfo*, SlotState*>> ret = last_player();
  if (!ret.has_value()) return std::nullopt;
  if (!ret->first->is_com()) return std::nullopt;
  return ret;
}

void CharMenuLogic::add_com() {
  if (max_player() <= static_cast<double>(players.size())) return;
  PlayerInfo* com = nullptr;
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (!kv.second->is_com()) continue;
    if (map_has(kv.second.get())) continue;
    com = kv.second.get();
  }
  if (com == nullptr) {
    com = lfw().player(u"com_" + lfw().new_id());
    com->set_name(Value(u"com"), false);
    com->set_is_com(true, false);
  }
  press_a(com);
}

SlotState* CharMenuLogic::find_state(std::vector<std::pair<PlayerInfo*, SlotState>>& map,
                                     PlayerInfo* player) {
  for (std::pair<PlayerInfo*, SlotState>& kv : map) {
    if (kv.first == player) return &kv.second;
  }
  return nullptr;
}

bool CharMenuLogic::map_has(PlayerInfo* player) {
  return find_state(players, player) != nullptr;
}

void CharMenuLogic::map_erase(std::vector<std::pair<PlayerInfo*, SlotState>>& map,
                              PlayerInfo* player) {
  for (size_t i = 0; i < map.size(); ++i) {
    if (map[i].first != player) continue;
    map.erase(map.begin() + static_cast<std::ptrdiff_t>(i));
    return;
  }
}

}
}
