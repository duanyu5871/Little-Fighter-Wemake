#include "lfw/lfw.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/bot/bot_controller.h"
#include "lfw/buff/buff.h"
#include "lfw/buff/registry.h"
#include "lfw/cmds/cmds.h"
#include "lfw/controller/ball_controller.h"
#include "lfw/controller/creators.h"
#include "lfw/core/js_string.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/game_key.h"
#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/loader/stage_val_getters.h"
#include "lfw/stage/entity_item.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/container_help/loop_offset.h"

namespace lfw {

namespace {

// TS 的 `DEFAULT_INFO`（`LFW._INFO` 的缺省值）。
const Value& default_info() {
  static const Value kDefault = [] {
    Object paths;
    (void)paths;
    auto arr = std::make_shared<Array>();
    arr->push_back(Value(std::u16string(u"prel.zip.json")));
    arr->push_back(Value(std::u16string(u"data.zip.json")));
    Object o;
    o.set(u"type", Value(std::u16string(u"FULL")));
    o.set(u"version", Value(0.0));
    o.set(u"title", Value(std::u16string(u"Little Fighter Wemake Origin Full Game")));
    o.set(u"description", Value(std::u16string(u"Little Fighter Wemake Origin Full Zip")));
    o.set(u"author", Value(std::u16string(u"Gim")));
    o.set(u"paths", Value(std::move(arr)));
    return Value(std::make_shared<Object>(o));
  }();
  return kDefault;
}

Value& info_ref() {
  static Value info = default_info();
  return info;
}

std::vector<LFW::ZipItem>& zips_ref() {
  static std::vector<LFW::ZipItem> zips = [] {
    std::vector<LFW::ZipItem> out;
    out.push_back(LFW::ZipItem{u"prel.zip.json", nullptr});
    out.push_back(LFW::ZipItem{u"data.zip.json", nullptr});
    return out;
  }();
  return zips;
}

std::vector<LFW*>& instances_ref() {
  static std::vector<LFW*> instances;
  return instances;
}

// `key_code in CMD_NAMES`：`CMD` 枚举值（含小写 `f1`… 与大写 `KILL_*` 等）的集合。
const std::vector<std::u16string>& cmd_names() {
  static const std::vector<std::u16string> kNames = {
      u"f1",    u"f2",    u"f3",    u"f4",    u"f5",    u"f6",    u"f7",    u"f8",
      u"f9",    u"f10",   u"LF2_NET", u"HERO_FT", u"GIM_INK", u"KILL_ENEMIES",
      u"KILL_BOSS", u"KILL_SOLDIERS", u"KILL_OTHERS", u"SET_PUPPET", u"DEL_PUPPET",
      u"SET_DIFFICULTY", u"DIST_CAM", u"LOCK_CAM", u"CHANGE_BG", u"CHANGE_STAGE",
      u"BGM",   u"PAUSE", u"SPAWN", u"DESPAWN", u"KILL", u"POINTER_DOWN", u"POINTER_MOVE",
      u"POINTER_UP", u"POINTER_CANCEL", u"POINTER_LEAVE", u"POINTER_ENTER",
      u"POINTER_CLICK", u"KEY_EVENT"};
  return kNames;
}

bool is_cmd_name(const std::u16string& key_code) {
  for (const std::u16string& n : cmd_names()) {
    if (n == key_code) return true;
  }
  return false;
}

// `String.prototype.startsWith`（空前缀恒真、长度不足恒假）。
bool js_starts_with(const std::u16string& s, const std::u16string& prefix) {
  if (prefix.size() > s.size()) return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (s[i] != prefix[i]) return false;
  }
  return true;
}

const std::u16string* as_str(const Value& v) { return std::get_if<std::u16string>(&v); }

std::u16string js_to_lower(const std::u16string& s) { return to_lower_case(s); }

// `Expressions<Stage>` 的元素：`new Expression(v, get_val_getter_from_stage)` 的包装。
class StageExpression : public stage::IExpression<stage::Stage> {
 public:
  explicit StageExpression(Expression<stage::Stage> e) : _e(std::move(e)) {}
  bool run(const stage::Stage& s) override { return _e.run(s); }

 private:
  Expression<stage::Stage> _e;
};

}  // namespace

std::u16string& LFW::VERSION_NAME() {
  static std::u16string name = u"v0.0.0";
  return name;
}

const std::u16string& LFW::DATA_TYPE() {
  static const std::u16string type = u"DataZip";
  return type;
}

double LFW::DATA_VERSION() {
  const Value* const v = defines::find(u"Defines.DATA_VERSION");
  return v != nullptr ? to_number(*v) : 0.0;
}

Value& LFW::INFO() { return info_ref(); }

void LFW::set_INFO(const Value* v) {
  const Value next = v != nullptr ? *v : default_info();
  if (equals(next, info_ref())) return;
  info_ref() = next;
  zips_ref().clear();
  // `this._ZIPS = this._INFO.paths`：paths 里是字符串。
  const Value* const paths = as_object(next) != nullptr ? as_object(next)->get(u"paths") : nullptr;
  if (paths != nullptr) {
    if (const Array* const arr = as_array(*paths)) {
      for (size_t i = 0; i < arr->size(); ++i) {
        const std::u16string* const s = as_str(arr->at(i));
        if (s != nullptr) zips_ref().push_back(ZipItem{*s, nullptr});
      }
    }
  }
}

bool LFW::IS_DEFAULT_INFO() {
  const Value* const has = &info_ref();
  return as_object(*has) != nullptr && equals(*has, default_info());
}

std::vector<LFW::ZipItem>& LFW::ZIPS() { return zips_ref(); }

void LFW::set_ZIPS(std::vector<ZipItem> v) {
  zips_ref() = std::move(v);
  for (LFW* const inst : instances_ref()) inst->update_zip_names();
}

std::vector<LFW*>& LFW::instances() { return instances_ref(); }

LFW* LFW::instance() { return instances_ref().empty() ? nullptr : instances_ref()[0]; }

World* LFW::world_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->world() : nullptr;
}

helper::ObjectsHelper* LFW::objects_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->objects_helper() : nullptr;
}

helper::ObjectsHelper* LFW::entities_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->entities_helper() : nullptr;
}

helper::CharactersHelper* LFW::fighters_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->characters_helper() : nullptr;
}

helper::WeaponsHelper* LFW::weapons_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->weapons_helper() : nullptr;
}

helper::BallsHelper* LFW::balls_s() {
  LFW* const inst = instance();
  return inst != nullptr ? &inst->balls_helper() : nullptr;
}

void LFW::IgnoreDisposed(const Value& e) {
  LFW* const inst = instance();
  if (inst != nullptr) {
    inst->host().warn({e});
  }
  const Value flag = field_or(e, u"is_disposed_error");
  if (std::holds_alternative<bool>(flag) && std::get<bool>(flag)) return;
  // TS 对非 disposed 错误会重新抛出；端口无异常 ⇒ 走宿主 error 通道（记偏差表）。
  if (inst != nullptr) {
    inst->host().error({e});
  }
}

LFW::LFW(ILfwHost& host, bool dev) : host_(&host), zips_(), _mt(host.now()) {
  dev_mode = dev;
  // `regist_components()` / `regist_buffs()`
  host_->regist_components();
  buff::regist_buffs();
  lfw_debug(u"constructor", {});

  resources_ = std::make_unique<Resources>(&zips_, this);
  datas_ = std::make_unique<loader::DatMgr>(this);
  host_->sounds_init(*this);
  host_->images_init(*this);
  host_->keyboard_init(*this);
  host_->keyboard_add_callback(*this);
  host_->pointings_init(*this);
  host_->cache_forget(DATA_TYPE(), DATA_VERSION());
  host_->cache_forget(u"PlayerInfo", PlayerInfo::kDataVersion);
  host_->zip_forget_stored(DATA_TYPE(), DATA_VERSION());

  fighters_ = std::make_unique<helper::CharactersHelper>(*this);
  weapons_ = std::make_unique<helper::WeaponsHelper>(*this);
  entities_ = std::make_unique<helper::ObjectsHelper>(*this);
  objects_ = std::make_unique<helper::ObjectsHelper>(*this);
  balls_ = std::make_unique<helper::BallsHelper>(*this);
  uis_ = std::make_unique<helper::UIHelper>(*this);

  for (const char16_t* const pid : {u"1", u"2", u"3", u"4", u"5", u"6", u"7", u"8"}) {
    const std::u16string id(pid);
    _players.push_back({id, std::make_unique<PlayerInfo>(this, id, Value(), Value(), Value())});
  }

  _states = std::make_unique<state::States>();
  world_ = std::make_unique<World>(*this, *host_->create_world_renderer(*this), _states.get());
  world_->start_update();
  world_->start_render();
  instances_ref().push_back(this);
  host_->pointings_add_ui_input(*this);
  _layers = host_->create_layers(*this);
  _layers->push();

  {
    // `this._i18n.add({'': {VERSION_NAME, DATA_LIST: ''}})`
    Object base;
    base.set(u"VERSION_NAME", Value(VERSION_NAME()));
    base.set(u"DATA_LIST", Value(std::u16string(u"")));
    Object langs;
    langs.set(u"", Value(std::make_shared<Object>(base)));
    _i18n.add(Value(std::make_shared<Object>(langs)));
  }
  update_zip_names();
}

LFW::~LFW() {
  std::vector<LFW*>& list = instances_ref();
  for (size_t i = 0; i < list.size();) {
    if (list[i] == this) list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
    else ++i;
  }
}

MersenneTwister& LFW::mt_ref() { return _mt; }

MersenneTwister* LFW::mt() { return &_mt; }

std::u16string LFW::new_id() {
  ++__id;
  return number_to_string(__id);
}

std::u16string LFW::new_team() {
  ++__team;
  return u"team_" + number_to_string(__team);
}

void LFW::reset_new_team() { __team = 8.0; }

void LFW::reset_new_id() { __id = 100.0; }

PlayerInfo* LFW::find_player(const std::u16string& player_id) const {
  for (const auto& kv : _players) {
    if (kv.first == player_id) return kv.second.get();
  }
  return nullptr;
}

PlayerInfo* LFW::player(const std::u16string& player_id) {
  PlayerInfo* ret = find_player(player_id);
  if (ret != nullptr) return ret;
  _players.push_back(
      {player_id, std::make_unique<PlayerInfo>(this, player_id, Value(), Value(), Value())});
  return _players.back().second.get();
}

PlayerInfo* LFW::player(const Value& player_id) { return player(to_string(player_id)); }

bool LFW::players_has(const Value& player_id) const {
  const std::u16string* const s = as_str(player_id);
  if (s == nullptr) return false;
  return find_player(*s) != nullptr;
}

bool LFW::set_player_bot(const std::u16string& player_id, bool bot) {
  PlayerInfo* const p = find_player(player_id);
  Entity* const fighter = p != nullptr ? p->fighter() : nullptr;
  if (fighter == nullptr) return false;
  if (bot) {
    if (fighter->ctrl() != nullptr && fighter->ctrl()->creator() == controller::bot_controller_creator()) {
      return true;
    }
    controller::BaseController* c =
        factory.create_ctrl(field_or(fighter->data(), u"id"), player_id, fighter);
    if (c == nullptr) c = controller::bot_controller_creator()->create(player_id, fighter);
    fighter->set_ctrl(c);
  } else {
    if (fighter->ctrl() != nullptr && fighter->ctrl()->is_human()) return true;
    controller::BaseController* const c =
        factory.acquire_ctrl(controller::local_controller_creator(), player_id, fighter);
    fighter->set_ctrl(c);
  }
  return true;
}

void LFW::random_entity_info(Entity& e) {
  const double l = world_->left();
  const double r = world_->right();
  const double n = world_->near_plane();
  const double f = world_->far_plane();
  e.id = new_id();
  e.facing = std::fmod(_mt.range(0.0, 100.0), 2.0) != 0.0 ? -1.0 : 1.0;
  const double x = _mt.range(l, r);
  const double z = _mt.range(f, n);
  e.position.set(x, 550.0, z);
}

bool LFW::is_cheat(const std::u16string& name) {
  if (!defines::is_cheat_type(name)) return false;
  return truthy(world_->dataset.get(name));
}

void LFW::set_cheat(const std::u16string& name, const std::optional<bool>& enable) {
  const bool cur = is_cheat(name);
  const bool en = enable.has_value() ? *enable : !cur;
  if (en == cur) return;
  push_cmd({name, en ? std::u16string(u"1") : std::u16string()});
  _cheat_keys.clear();
  _cheat_gkeys.clear();
}

void LFW::on_key_down(LfwKeyEvent& e) {
  {
    std::vector<Value> args;
    Object ev;
    ev.set(u"key", Value(e.key));
    ev.set(u"times", Value(e.times));
    ev.set(u"device_type", Value(e.device_type));
    args.push_back(Value(std::make_shared<Object>(ev)));
    lfw_debug(u"on_key_down", args);
  }
  const std::u16string key_code = js_to_lower(e.key);
  if (is_cmd_name(key_code)) {
    push_cmd({key_code});
    e.interrupt();
  }

  if (e.times == 0.0) {
    for (const char16_t* const key_name : all_game_keys()) {
      for (auto& kv : _players) {
        PlayerInfo* const player = kv.second.get();
        if (!truthy(player->local())) continue;
        const Value keys = player->keys();
        const Value cur = field_or(keys, key_name);
        const std::u16string* const cur_s = as_str(cur);
        if (cur_s == nullptr || *cur_s != key_code) continue;
        if (e.device_type == u"controller") {
          LfwCallbackArgs args;
          args.lfw = this;
          args.player = player;
          callbacks.call(u"controller_detected", {args});
        }
        if (e.device_type == u"keyboard") {
          LfwCallbackArgs args;
          args.lfw = this;
          args.player = player;
          callbacks.call(u"keyboard_detected", {args});
        }
        std::u16string* g = nullptr;
        for (auto& gkv : _cheat_gkeys) {
          if (gkv.first == kv.first) {
            g = &gkv.second;
            break;
          }
        }
        if (g == nullptr) {
          _cheat_gkeys.push_back({kv.first, std::u16string()});
          g = &_cheat_gkeys.back().second;
        }
        *g += key_name;
        push_cmd({u"KEY_EVENT", u"--p=" + kv.first, u"--s=1", u"--c=" + key_code,
                  u"--n=" + std::u16string(key_name)});
      }
    }
  }

  bool match = false;
  _cheat_gkeys_matchs.clear();
  _cheat_keys += key_code;
  const Value* const infos = defines::find(u"Defines.CheatInfos");
  if (infos != nullptr) {
    const Object* const info_obj = as_object(*infos);
    if (info_obj != nullptr) {
      for (const std::u16string& cheat_name : info_obj->keys()) {
        const Value* const cheat = info_obj->get(cheat_name);
        if (cheat == nullptr) continue;
        const Value k = field_or(*cheat, u"keys");
        const Value g = field_or(*cheat, u"gkeys");
        const std::u16string* const k_s = as_str(k);
        const std::u16string* const g_s = as_str(g);
        for (const auto& pid_gkeys : _cheat_gkeys) {
          if (g_s != nullptr && js_starts_with(*g_s, pid_gkeys.second)) {
            bool has = false;
            for (const std::u16string& m : _cheat_gkeys_matchs) {
              if (m == pid_gkeys.first) has = true;
            }
            if (!has) _cheat_gkeys_matchs.push_back(pid_gkeys.first);
          }
          if (g_s != nullptr && *g_s == pid_gkeys.second) set_cheat(cheat_name);
        }
        if (k_s != nullptr && js_starts_with(*k_s, _cheat_keys)) match = true;
        if (k_s != nullptr && *k_s == _cheat_keys) set_cheat(cheat_name);
      }
    }
  }
  {
    const std::vector<std::pair<std::u16string, std::u16string>> snapshot = _cheat_gkeys;
    for (const auto& kv : snapshot) {
      bool has = false;
      for (const std::u16string& m : _cheat_gkeys_matchs) {
        if (m == kv.first) has = true;
      }
      if (!has) {
        for (size_t i = 0; i < _cheat_gkeys.size();) {
          if (_cheat_gkeys[i].first == kv.first) {
            _cheat_gkeys.erase(_cheat_gkeys.begin() + static_cast<std::ptrdiff_t>(i));
          } else {
            ++i;
          }
        }
      }
    }
  }
  if (!match) _cheat_keys.clear();
}

void LFW::on_key_up(LfwKeyEvent& e) {
  const std::u16string key_code = e.key.empty() ? std::u16string() : js_to_lower(e.key);
  for (const char16_t* const key_name : all_game_keys()) {
    for (auto& kv : _players) {
      PlayerInfo* const player = kv.second.get();
      if (!truthy(player->local())) continue;
      const Value cur = field_or(player->keys(), key_name);
      const std::u16string* const cur_s = as_str(cur);
      if (cur_s == nullptr || *cur_s != key_code) continue;
      push_cmd({u"KEY_EVENT", u"--p=" + kv.first, u"--s=0", u"--c=" + key_code,
                u"--n=" + std::u16string(key_name)});
    }
  }
}

LFW& LFW::push_cmd(const std::vector<std::u16string>& words) {
  std::u16string joined;
  for (size_t i = 0; i < words.size(); ++i) {
    if (i != 0) joined.push_back(u' ');
    joined += words[i];
  }
  cmds.push_back(std::move(joined));
  return *this;
}

void LFW::dispose() {
  lfw_debug(u"dispose", {});
  _disposed = true;
  callbacks.call(u"on_dispose", {LfwCallbackArgs{this}});
  callbacks.clear();
  world_->dispose();
  datas_->dispose();
  host_->sounds_dispose();
  host_->keyboard_dispose();
  host_->pointings_dispose();
  _layers->dispose();
  std::vector<LFW*>& list = instances_ref();
  for (size_t i = 0; i < list.size();) {
    if (list[i] == this) list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
    else ++i;
  }
}

void LFW::change_bg(const std::u16string& bg) { world_->change_bg(Value(bg)); }

void LFW::change_stage(const std::u16string& stage) { world_->change_stage(Value(stage)); }

void LFW::goto_next_stage() {
  lfw_debug(u"goto_next_stage", {});
  Stage* stage = world_->stage();
  const Value next = stage != nullptr ? field_or(stage->data(), u"next") : Value();
  if (!truthy(next)) return;
  const std::u16string* const next_s = as_str(next);
  if (next_s != nullptr && *next_s == u"end") {
    _layers->set_page(Value(std::make_shared<Object>([] {
                       Object o;
                       o.set(u"id", Value(std::u16string(u"ending_page")));
                       return o;
                     }())),
                      0.0);
    return;
  }
  const Value* next_stage = nullptr;
  for (const Value& s : datas_->stages()) {
    if (equals(field_or(s, u"id"), next)) next_stage = &s;
  }
  if (next_stage == nullptr) {
    stage->stop_bgm();
    const Value* const pass_sound = defines::find(u"Defines.Sounds.StagePass");
    sounds_play_with_load(pass_sound != nullptr ? *pass_sound : Value());
    callbacks.call(u"on_stage_pass", {LfwCallbackArgs{this}});
  }
  if (next_stage != nullptr && truthy(field_or(*next_stage, u"is_starting"))) {
    for (Entity* const e : world_->entities) {
      if (entity::is_fighter(e->ref()) && e->ctrl() != nullptr &&
          players_has(Value(e->ctrl()->player_id))) {
        continue;
      }
      e->release();
    }
    for (Entity* const e : world_->ghosts) e->release();
  }
  const double time = stage->time();
  change_stage(next_stage != nullptr ? to_string(field_or(*next_stage, u"id")) : std::u16string());
  world_->stage()->set_time(time);
  callbacks.call(u"on_enter_next_stage", {LfwCallbackArgs{this}});
}

Value LFW::string(const Value& name) const {
  return _i18n.string(name, Value(_i18n.lang()));
}

Value LFW::strings(const Value& name) const {
  return _i18n.strings(name, Value(_i18n.lang()));
}

void LFW::set_lang(const Value& lang) {
  const std::u16string* const s = as_str(lang);
  if (s == nullptr) {
    lfw_warn(u"set_lang", {Value(u"lang should be string, but got " + to_string(lang))});
    return;
  }
  const std::u16string prev = _i18n.lang();
  if (prev == *s) return;
  host_->lang_apply(*this, *s, prev);
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = *s;
  args.prev_text = prev;
  callbacks.call(u"on_lang_changed", {args});
}

Value LFW::canonical_lang(const std::optional<std::u16string>& lang) const {
  return _i18n.canonical(Value(lang.has_value() ? *lang : _i18n.lang()));
}

void LFW::emit_progress(const std::u16string& content, double progress) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content;
  args.num = progress;
  callbacks.call(u"on_progress", {args});
}

void LFW::emit_progress_size(const std::u16string& content, double progress, const Value& size) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = content;
  args.num = progress;
  args.num2 = to_number(size);
  args.has_num2 = true;
  callbacks.call(u"on_progress", {args});
}

void LFW::broadcast(const Value& m) {
  broadcasts.push_back(to_string(m));
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = to_string(m);
  callbacks.call(u"on_broadcast", {args});
}

void LFW::on_component_broadcast(ui::UIComponent* component, const std::u16string& message) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.component = component;
  args.text = message;
  callbacks.call(u"on_component_broadcast", {args});
}

void LFW::switch_difficulty(double offset) {
  std::vector<Value> full = {Value(1.0), Value(2.0), Value(3.0)};
  if (is_cheat(u"LF2_NET")) full.push_back(Value(4.0));
  const Value current = world_->dataset.get(u"difficulty");
  // `loop_offset(list, dataset.difficulty, offset)`：`===` 比较（Value 没有 `operator==`
  // ⇒ 这里手写一份，语义照 `utils/container_help/loop_offset.h`）。
  const size_t len = full.size();
  double idx = -1.0;
  for (size_t i = 0; i < len; ++i) {
    if (strict_equals(full[i], current)) {
      idx = static_cast<double>(i);
      break;
    }
  }
  const std::u16string next_text = [&]() -> std::u16string {
    if (len == 0) return u"undefined";
    double off = std::fmod(offset, static_cast<double>(len));
    if (off > 0.0) {
      idx = std::fmod(idx + off, static_cast<double>(len));
    } else {
      idx = std::fmod(static_cast<double>(len) + idx + off, static_cast<double>(len));
    }
    if (!(idx >= 0.0) || idx != std::floor(idx) || idx >= static_cast<double>(len)) {
      return u"undefined";
    }
    return number_to_string(to_number(full[static_cast<size_t>(idx)]));
  }();
  push_cmd({u"SET_DIFFICULTY", next_text});
}

void LFW::update_zip_names() {
  std::vector<Value> data_list;
  const std::vector<ZipItem>& zips = zips_ref();
  for (size_t i = 2; i < zips.size(); ++i) {
    data_list.push_back(Value(zips[i].is_zip() ? zips[i].zip->name() : zips[i].path));
  }
  if (!IS_DEFAULT_INFO()) {
    data_list.insert(data_list.begin(), field_or(info_ref(), u"title"));
  }
  auto arr = std::make_shared<Array>();
  for (Value& v : data_list) arr->push_back(std::move(v));
  Object base;
  base.set(u"DATA_LIST", Value(std::move(arr)));
  Object langs;
  langs.set(u"", Value(std::make_shared<Object>(base)));
  _i18n.add(Value(std::make_shared<Object>(langs)));
  callbacks.call(u"on_extra_zips_changed", {LfwCallbackArgs{this}});
}

void LFW::set_survival_rank_data(const Value& data) {
  survival_rank_data = data;
  LfwCallbackArgs args;
  args.lfw = this;
  args.value = data;
  callbacks.call(u"on_survival_rank_changed", {args});
}

bool LFW::survival_rank_cheated() {
  return is_cheat(u"LF2_NET") || is_cheat(u"HERO_FT") || is_cheat(u"GIM_INK");
}

bool LFW::survival_rank_modded() { return zips_ref().size() > 2; }

bool LFW::survival_rank_invalid() { return survival_rank_cheated() || survival_rank_modded(); }

void LFW::lfw_debug(const std::u16string& func, const std::vector<Value>& args) {
  if (!__debugging) return;
  std::vector<Value> out;
  out.push_back(Value(u"[D][LFW::" + func + u"]"));
  for (const Value& a : args) out.push_back(a);
  host_->debug_msg(out);
}

void LFW::lfw_warn(const std::u16string& func, const std::vector<Value>& args) {
  std::vector<Value> out;
  out.push_back(Value(u"[W][LFW::" + func + u"]"));
  for (const Value& a : args) out.push_back(a);
  host_->warn(out);
}

void LFW::lfw_log(const std::u16string& func, const std::vector<Value>& args) {
  std::vector<Value> out;
  out.push_back(Value(u"[I][LFW::" + func + u"]"));
  for (const Value& a : args) out.push_back(a);
  host_->log(out);
}

void LFW::ui_changed(ui::UINode* curr, ui::UINode* prev) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.curr = curr;
  args.prev = prev;
  callbacks.call(u"on_ui_changed", {args});
}

Keys* LFW::keys() {
  if (_keys == nullptr) _keys = create_keys();
  return _keys;
}

Keys* LFW::create_keys() {
  Keys* ret = nullptr;
  const std::optional<Keys*> taken = _keys_graves.take();
  ret = taken.has_value() ? *taken : new Keys(*this);
  ret->mount();
  return ret;
}

void LFW::regist_keys(Keys& keys) {
  for (Keys* const k : mounted_keys) {
    if (k == &keys) {
      lfw_warn(u"regist_keys", {Value(u"keys already registered")});
      return;
    }
  }
  mounted_keys.push_back(&keys);
}

void LFW::recycle_keys(Keys& keys) {
  bool ok = false;
  for (size_t i = 0; i < mounted_keys.size();) {
    if (mounted_keys[i] == &keys) {
      mounted_keys.erase(mounted_keys.begin() + static_cast<std::ptrdiff_t>(i));
      ok = true;
    } else {
      ++i;
    }
  }
  if (!ok) {
    lfw_warn(u"recycle_keys", {Value(u"keys not found!")});
    return;
  }
  _keys_graves.add(&keys);
}

collision::Collision* LFW::acquire_collision() {
  const std::optional<collision::Collision*> taken = _collision_graves.take();
  return taken.has_value() ? *taken : nullptr;
}

void LFW::recycle_collision(collision::Collision* c) { _collision_graves.add(c); }

double LFW::lifetime() { return world_->lifetime(); }

void LFW::load_img(const std::u16string& path) { host_->load_img(path); }

void LFW::warn(const std::vector<Value>& args) { host_->warn(args); }

void LFW::error(const std::vector<Value>& args) { host_->error(args); }

bool LFW::import_as_json(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                         std::u16string& error) {
  return host_->import_as_json(urls, data, hit, error);
}

bool LFW::import_as_blob_url(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                             std::u16string& error) {
  return host_->import_as_blob_url(urls, data, hit, error);
}

bool LFW::import_as_array_buffer(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                                 std::u16string& error) {
  return host_->import_as_array_buffer(urls, data, hit, error);
}

bool LFW::import_as_image_bitmap(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                                 std::u16string& error) {
  return host_->import_as_image_bitmap(urls, data, hit, error);
}

bool LFW::import_as_text(const std::vector<std::u16string>& urls, Value& data, Value& hit,
                         std::u16string& error) {
  return host_->import_as_text(urls, data, hit, error);
}

bool LFW::xml_parse(const Value& text, Value& marker, std::shared_ptr<IXMLElement>& root,
                    std::u16string& error) {
  return host_->xml_parse(text, marker, root, error);
}

void LFW::cache_get(const std::u16string& name, PlayerInfoCacheEntry& out) {
  host_->player_cache_get(name, out);
}

bool LFW::cache_del(const std::u16string& name, std::u16string& error) {
  return host_->player_cache_del(name, error);
}

void LFW::cache_put(const PlayerInfoCachePut& data) { host_->player_cache_put(data); }

void LFW::warn(const std::u16string& text) {
  std::vector<Value> args;
  args.push_back(Value(text));
  host_->warn(args);
}

const std::vector<Entity*>& LFW::world_entities() { return world_->entities; }

const std::vector<Entity*>& LFW::world_ghosts() { return world_->ghosts; }

void LFW::del_entities(const std::vector<Entity*>& list) { world_->del_entities(list); }

Entity* LFW::create_entity(const Value& data) {
  return factory.create_entity(world_.get(), data, _states.get());
}

controller::BaseController* LFW::create_ctrl(const Value& oid, const std::u16string& player_id,
                                             Entity* entity) {
  return factory.create_ctrl(oid, player_id, entity);
}

const Value* LFW::find_fighter(const Value& id) { return datas_->find_fighter(id); }

const Value* LFW::find_weapon(const Value& id) { return datas_->find_weapon(id); }

const std::vector<Value>& LFW::fighters() { return datas_->fighters(); }

const std::vector<Value>& LFW::weapons() { return datas_->weapons(); }

void LFW::push_page(const Value& page, double stack_idx) { _layers->push_page(page, stack_idx); }

void LFW::set_page(const Value& page, double stack_idx) { _layers->set_page(page, stack_idx); }

Value LFW::datas_backgrounds_find(const Value& id) {
  const Value* const found = datas_->find_background(id);
  return found != nullptr ? *found : Value();
}

Value LFW::datas_stages_find(const Value& id) {
  for (const Value& s : datas_->stages()) {
    if (equals(field_or(s, u"id"), id)) return s;
  }
  return Value();
}

std::function<void()> LFW::sounds_play_bgm(const Value& music) {
  return host_->sounds_play_bgm(music);
}
void LFW::sounds_stop_bgm() { host_->sounds_stop_bgm(); }

void LFW::sounds_play(const Value& path, const Value& x, const Value& y, const Value& z) {
  host_->sounds_play(path, x, y, z);
}

void LFW::sounds_play_with_load(const Value& path) { host_->sounds_play_with_load(path); }

stage::Expressions<stage::Stage>::Items LFW::end_testers(const Value& owner) {
  stage::Expressions<stage::Stage>::Items out;
  const Value end_test = field_or(owner, u"end_test");
  const Array* const arr = as_array(end_test);
  if (arr == nullptr) return out;
  for (size_t i = 0; i < arr->size(); ++i) {
    const std::u16string* const s = as_str(arr->at(i));
    if (s == nullptr) continue;
    auto expr = std::make_unique<Expression<stage::Stage>>(*s, &loader::get_val_getter_from_stage);
    out.push_back(std::make_shared<StageExpression>(std::move(*expr)));
  }
  return out;
}

bool LFW::keys_is_start(const std::u16string& key) {
  Keys* const k = keys();
  controller::KeyStatus* const st = k->get(key);
  if (st == nullptr) return false;
  return st->is_start(k->time());
}

Value LFW::datas_find(const Value& oid) {
  const Value* const found = datas_->find(oid);
  return found != nullptr ? *found : Value();
}

std::shared_ptr<Randoming> LFW::datas_randoming_by_group(const Value& oid) {
  return datas_->get_randoming_by_group(to_string(oid));
}

stage::IItemEntity* LFW::create_entity_with_bot(const Value& data) {
  Entity* const e = factory.create_entity_with_bot(std::u16string(), world_.get(), data,
                                                   _states.get());
  if (e == nullptr) return nullptr;
  auto item = std::make_unique<stage::EntityItem>(*e);
  stage::EntityItem* const ret = item.get();
  entity_items_.push_back(std::move(item));
  return ret;
}

Value LFW::get_random_bg(const std::vector<Value>& groups) {
  std::vector<std::u16string> names;
  for (const Value& v : groups) names.push_back(to_string(v));
  return datas_->get_random_bg(names);
}

Entity* LFW::create_entity(World& world, const Value& data) {
  return factory.create_entity(&world, data, world.states());
}

Entity* LFW::create_entity_with_player(const std::u16string& player_id, World& world,
                                       const Value& data) {
  return factory.create_entity_with_player(player_id, &world, data, world.states());
}

Entity* LFW::create_entity_with_bot(const std::u16string& player_id, World& world,
                                    const Value& data) {
  return factory.create_entity_with_bot(player_id, &world, data, world.states());
}

void LFW::recycle_entity(Entity* e) { factory.recycle_entity(e); }

void LFW::recycle_buff(buff::Buff* b) { factory.recycle_buff(b); }

controller::BaseController* LFW::acquire_invalid_ctrl(World& world) {
  (void)world;
  return factory.acquire_ctrl(controller::invalid_controller_creator(), std::u16string(), nullptr);
}

void LFW::release_ctrl(controller::BaseController* ctrl) { factory.release_ctrl(ctrl); }

controller::BaseController* LFW::acquire_local_ctrl(const std::u16string& player_id,
                                                    Entity& entity) {
  return factory.acquire_ctrl(controller::local_controller_creator(), player_id, &entity);
}

Value LFW::datas_fighters_find(const Value& oid) {
  const Value* const found = datas_->find_fighter(oid);
  return found != nullptr ? *found : Value();
}

Value LFW::datas_weapons_of_group(const Value& group) {
  const std::vector<Value> list = datas_->get_weapons_of_group(to_string(group));
  auto arr = std::make_shared<Array>();
  for (const Value& v : list) arr->push_back(v);
  return Value(std::move(arr));
}

void LFW::entities_add(const Value& data, double num) { entities_->add(data, num); }

void LFW::cheat_changed(const std::u16string& cmd, bool enabled) {
  LfwCallbackArgs args;
  args.lfw = this;
  args.text = cmd;
  args.flag = enabled;
  callbacks.call(u"on_cheat_changed", {args});
}

std::vector<IWorldUi*> LFW::layer_uis() { return _layers->layer_uis(); }

double LFW::mt_range(double min_v, double max_v) { return _mt.range(min_v, max_v); }

Value LFW::find_bot(const std::u16string& bot_id) const {
  const Value* const found = datas_->find_bot(bot_id);
  return found != nullptr ? *found : Value();
}

buff::Buff* LFW::create_buff(const std::u16string& kind, const std::u16string& id) {
  return factory.create_buff(Value(kind), this, id);
}

controller::BaseController* LFW::create_ctrl(const std::u16string& data_id,
                                             const std::u16string& player_id) {
  return factory.create_ctrl(Value(data_id), player_id, nullptr);
}

void LFW::ctrl_update_lookup(controller::BaseController& ctrl, double index,
                             std::vector<Entity*>& entities) {
  std::vector<Value> vals;
  for (Entity* const e : entities) vals.push_back(e->ref());
  if (ctrl.creator() == controller::bot_controller_creator()) {
    static_cast<bot::BotController*>(&ctrl)->update_lookup(static_cast<int>(index), vals);
  } else if (ctrl.creator() == controller::ball_controller_creator()) {
    static_cast<controller::BallController*>(&ctrl)->update_lookup(static_cast<int>(index), vals);
  }
}

void LFW::debug(const std::u16string& msg) {
  std::vector<Value> args;
  args.push_back(Value(msg));
  host_->debug_msg(args);
}

void LFW::handle_cmds(World& world) { cmds::CMDS::handle(world, cmds); }

void LFW::ctrl_come(controller::BaseController& ctrl, double x, double y, double z) {
  if (ctrl.creator() == controller::bot_controller_creator()) {
    static_cast<bot::BotController*>(&ctrl)->come(x, y, z);
  }
}

void LFW::ctrl_move(controller::BaseController& ctrl) {
  if (ctrl.creator() == controller::bot_controller_creator()) {
    static_cast<bot::BotController*>(&ctrl)->move();
  }
}

void LFW::ctrl_stay(controller::BaseController& ctrl) {
  if (ctrl.creator() == controller::bot_controller_creator()) {
    static_cast<bot::BotController*>(&ctrl)->stay();
  }
}

void LFW::ctrl_follow(controller::BaseController& ctrl, Entity& target) {
  if (ctrl.creator() == controller::bot_controller_creator()) {
    static_cast<bot::BotController*>(&ctrl)->follow(target.ref());
  }
}

bool LFW::ctrl_goingto(const controller::BaseController& ctrl) const {
  if (ctrl.creator() != controller::bot_controller_creator()) return false;
  const bot::BotController* const b = static_cast<const bot::BotController*>(&ctrl);
  return truthy(b->goingto);
}

}  // namespace lfw
