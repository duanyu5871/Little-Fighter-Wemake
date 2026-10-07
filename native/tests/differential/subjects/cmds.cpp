// `cmds/` 家族（4X/4Y）的 C++ 侧台面：`CMDS` 的解析/注册表 + 已移植 27 条命令的效果。
// TS 侧是 `subjects/cmds.ts`，op 一一对应。用例：`cases/cmds/*.txt`。
//
// 台面私货：两侧同样注册一条 `__probe__` 命令，把 `ctx.words` / `positionals` / `str` / `num` /
// `nums` / `*_arg` 的结果打出来（解析层全靠它做差分）。命令用 `cmd <字符串字面量>` 派发。
//
// op：
//   wnew                                建世界（假 `lfw` + 假 renderer）
//   hdump                               世界摘要（cmds 观测量：dataset / 相机 / 实体 / 傀儡 / 计数）
//   ds <键> <值>                        `dataset.set`
//   bdata <值> | sdata <值>             假 `lfw.datas` 的背景 / 舞台登记
//   data <值> | fdata <值> | wdata <值> 假 `lfw.datas` 的 find / fighters / weapons 表
//   player <pid> <名字>                 假 `lfw.players` 登记
//   bg <值> | stage <值>                `world.change_bg` / `world.change_stage`
//   cheat <0|1>                         假 `lfw.is_cheat(HERO_FT)`（`stage_limit` 的另一半）
//   mk <标签> <数据> | add <标签> <数据>  建实体（后一个再 `add_entities`）
//   ent <标签> hp|hpr|mp|team <值>       改实体字段
//   pup <player_id> <标签>              往 `world.puppets` 登记傀儡
//   ctrl <标签> human|base|none [pid]   换实体的控制器（假 `make_ctrl`）
//   cmd <字符串字面量>                   `CMDS::handle(world, [str])`（`__probe__` 也在里面）
//   wcmds <字符串字面量>…                假 `lfw` 的 cmds 列表
//   handlecmds                          `world.handle_cmds()`（走 `IWorldLfw::handle_cmds` 缝）
//   h <字符串字面量>                     `CMDS::handler(key)` 有没有（1/0）
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/camera.h"
#include "lfw/cmds/cmds.h"
#include "lfw/core/value.h"
#include "lfw/defines/cheat_type.h"
#include "lfw/entity/entity.h"
#include "lfw/entity/entity_ref.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/player_info.h"
#include "lfw/stage/stage.h"
#include "lfw/state/states.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/mersenne_twister.h"
#include "lfw/world.h"

#include "trace_util.h"

namespace {

using lfw::Value;
using lfw::World;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

std::vector<std::string> g_out;

void push(const std::string& line) { g_out.push_back(line); }

std::string num(double d) { return trace::num_hex(d); }
std::string vstr(const Value& v) { return to_ascii(render_value(v)); }
std::string flag(bool b) { return b ? "1" : "0"; }
std::string esc(const std::u16string& s) { return trace::esc(s); }

lfw::state::States g_states;

// ---------------------------------------------------------------- 假件

class FakeRenderer : public lfw::IWorldRenderer {
 public:
  void add_entity(lfw::Entity& e) override { push("h:radd=" + to_ascii(e.id)); }
  void del_entity(lfw::Entity& e) override { push("h:rdel=" + to_ascii(e.id)); }
  void render(double dt) override { push("h:render=" + num(dt)); }
  void dispose() override { push("h:rdispose"); }
};

FakeRenderer g_renderer;

lfw::MersenneTwister g_mt{0.0};

std::vector<std::unique_ptr<lfw::Entity>> g_created;
std::vector<std::u16string> g_cmds;
std::vector<Value> g_bg_datas;
std::vector<Value> g_stage_datas;
std::vector<Value> g_broadcasts;
std::vector<std::string> g_cheats;
std::vector<Value> g_datas;
std::vector<Value> g_fighter_datas;
std::vector<Value> g_weapon_datas;
std::vector<std::unique_ptr<lfw::PlayerInfo>> g_players;
std::vector<std::pair<std::string, lfw::PlayerInfo*>> g_player_map;
int g_id_counter = 0;
int g_team_counter = 0;
std::unique_ptr<World> g_world;

class FakePlayerHost : public lfw::IPlayerInfoHost {
 public:
  void cache_get(const std::u16string&, lfw::PlayerInfoCacheEntry& out) override {
    out.missing = true;
  }
  bool cache_del(const std::u16string&, std::u16string&) override { return true; }
  void cache_put(const lfw::PlayerInfoCachePut&) override {}
  void warn(const std::u16string& text) override { push("h:piwarn=" + esc(text)); }
};

FakePlayerHost g_player_host;

std::vector<std::unique_ptr<lfw::controller::BaseController>> g_ctrls;

lfw::controller::BaseController* make_ctrl(const std::string& kind) {
  if (kind == "none") return nullptr;
  auto c = std::make_unique<lfw::controller::BaseController>();
  c->set_kind(kind == "human", kind == "bot");
  c->set_ball(kind == "ball");
  c->player_id = kind == "base" ? std::u16string() : u"7";
  lfw::Object player;
  player.set(u"id", Value(7.0));
  player.set(u"name", Value(std::u16string(u"P7")));
  player.set(u"mine", Value(true));
  c->player = Value(std::make_shared<lfw::Object>(player));
  lfw::controller::BaseController* raw = c.get();
  g_ctrls.push_back(std::move(c));
  return raw;
}

// 台面实体工厂：`add` op 与 `create_entity*` 缝共用（新实体的 id 来自假 `new_id`）。
lfw::Entity* new_fake_entity(World& world, const Value& data) {
  auto e = std::make_unique<lfw::Entity>(world.host(), data, &g_states);
  lfw::Entity* const raw = e.get();
  g_created.push_back(std::move(e));
  return raw;
}

class FakeLfw : public lfw::IWorldLfw {
 public:
  // ---- `IStageLfw` ----
  lfw::MersenneTwister* mt() override { return &g_mt; }

  Value datas_backgrounds_find(const Value& id) override {
    for (const Value& d : g_bg_datas) {
      if (lfw::strict_equals(lfw::field_or(d, u"id"), id)) {
        push("h:bgfind=" + vstr(lfw::field_or(d, u"id")));
        return d;
      }
    }
    push("h:bgfind=u");
    return Value();
  }

  Value datas_stages_find(const Value& id) override {
    for (const Value& d : g_stage_datas) {
      if (lfw::strict_equals(lfw::field_or(d, u"id"), id)) {
        push("h:stagefind=" + vstr(lfw::field_or(d, u"id")));
        return d;
      }
    }
    push("h:stagefind=u");
    return Value();
  }

  std::u16string new_team() override {
    g_team_counter += 1;
    return u"t" + to_u16(std::to_string(g_team_counter));
  }

  bool players_has(const Value& player_id) const override {
    (void)player_id;
    return false;
  }

  std::function<void()> sounds_play_bgm(const Value& music) override {
    push("h:playbgm=" + vstr(music));
    return []() { push("h:stopbgm"); };
  }

  void sounds_stop_bgm() override { push("h:stopbgm_now"); }

  void sounds_play(const Value& path, const Value& x, const Value& y, const Value& z) override {
    push("h:sound=" + vstr(path) + "," + vstr(x) + "," + vstr(y) + "," + vstr(z));
  }

  void sounds_play_with_load(const Value& path) override {
    push("h:loadplay=" + vstr(path));
  }

  lfw::stage::Expressions<lfw::stage::Stage>::Items end_testers(const Value& owner) override {
    (void)owner;
    return {};
  }

  Value datas_find(const Value& oid) override {
    push("h:datasfind=" + vstr(oid));
    for (const Value& d : g_datas) {
      if (lfw::strict_equals(lfw::field_or(d, u"id"), oid)) return d;
    }
    return Value();
  }

  std::shared_ptr<lfw::Randoming> datas_randoming_by_group(const Value& oid) override {
    (void)oid;
    return nullptr;
  }

  lfw::stage::IItemEntity* create_entity_with_bot(const Value& data) override {
    (void)data;
    return nullptr;
  }

  // ---- `IWorldLfw` ----
  lfw::PlayerInfo* player(const Value& player_id) override {
    push("h:player=" + vstr(player_id));
    for (const std::pair<std::string, lfw::PlayerInfo*>& kv : g_player_map) {
      if (lfw::strict_equals(Value(to_u16(kv.first)), player_id)) return kv.second;
    }
    return nullptr;
  }

  Value get_random_bg(const std::vector<Value>& groups) override {
    std::string list;
    for (const Value& g : groups) {
      if (!list.empty()) list += ",";
      list += vstr(g);
    }
    push("h:randbg=" + list);
    return Value();
  }

  lfw::Entity* create_entity(World& world, const Value& data) override {
    lfw::Entity* const raw = new_fake_entity(world, data);
    push("h:create=" + to_ascii(raw->id));
    return raw;
  }

  lfw::Entity* create_entity_with_player(const std::u16string& player_id, World& world,
                                         const Value& data) override {
    push("h:ceplayer=" + vstr(Value(player_id)));
    return new_fake_entity(world, data);
  }

  lfw::Entity* create_entity_with_bot(const std::u16string& player_id, World& world,
                                      const Value& data) override {
    push("h:cebot=" + vstr(Value(player_id)));
    return new_fake_entity(world, data);
  }

  lfw::Value datas_fighters_find(const lfw::Value& oid) override {
    for (const lfw::Value& d : g_fighter_datas) {
      if (lfw::strict_equals(lfw::field_or(d, u"id"), oid)) {
        push("h:fdatafind=" + vstr(oid));
        return d;
      }
    }
    push("h:fdatafind=u");
    return lfw::Value();
  }

  lfw::Value datas_weapons_of_group(const lfw::Value& group) override {
    push("h:wpgroup=" + vstr(group));
    std::shared_ptr<lfw::Array> arr = std::make_shared<lfw::Array>();
    for (const lfw::Value& d : g_weapon_datas) arr->push_back(d);
    return lfw::Value(arr);
  }

  void entities_add(const lfw::Value& data, double n) override {
    push("h:entadd=" + vstr(lfw::field_or(data, u"id")) + "|" + trace::num_hex(n));
  }

  void random_entity_info(lfw::Entity& e) override { push("h:randominfo=" + to_ascii(e.id)); }

  void cheat_changed(const std::u16string& cmd, bool enabled) override {
    push("h:cheatchanged=" + vstr(Value(cmd)) + "|" + (enabled ? "1" : "0"));
  }

  void recycle_entity(lfw::Entity* e) override {
    push(std::string("h:recycle=") + (e != nullptr ? to_ascii(e->id) : std::string("z")));
  }

  void recycle_buff(lfw::buff::Buff* b) override {
    (void)b;
    push("h:recyclebuff");
  }

  lfw::controller::BaseController* acquire_invalid_ctrl(World& world) override {
    (void)world;
    push("h:acquire");
    return make_ctrl("base");
  }

  lfw::controller::BaseController* acquire_local_ctrl(const std::u16string& player_id,
                                                      lfw::Entity& entity) override {
    (void)entity;
    push("h:acqlocal=" + vstr(Value(player_id)));
    lfw::controller::BaseController* const c = make_ctrl("human");
    c->player_id = player_id;
    return c;
  }

  void release_ctrl(lfw::controller::BaseController* ctrl) override {
    (void)ctrl;
    push("h:release");
  }

  std::vector<lfw::IWorldUi*> layer_uis() override { return {}; }

  double mt_range(double min_v, double max_v) override { return g_mt.range(min_v, max_v); }

  bool is_cheat(const std::u16string& name) override {
    push("h:cheat=" + vstr(Value(name)));
    for (const std::string& n : g_cheats) {
      if (n == to_ascii(name)) return true;
    }
    return false;
  }

  std::u16string new_id() override {
    g_id_counter += 1;
    push("h:newid=" + std::to_string(g_id_counter));
    return u"e" + to_u16(std::to_string(g_id_counter));
  }

  void broadcast(const Value& m) override { push("h:broadcast=" + vstr(m)); }

  lfw::controller::BaseController* create_ctrl(const std::u16string& data_id,
                                               const std::u16string& player_id) override {
    push("h:createctrl=" + vstr(Value(data_id)) + ":" + vstr(Value(player_id)));
    return nullptr;
  }

  bool has_cmds() const override { return !g_cmds.empty(); }
  void clear_cmds() override { g_cmds.clear(); }
  void clear_broadcasts() override { g_broadcasts.clear(); }

  void ctrl_update_lookup(lfw::controller::BaseController& ctrl, double index,
                          std::vector<lfw::Entity*>& entities) override {
    (void)ctrl;
    (void)entities;
    push("h:lookup=" + num(index));
  }

  bool dev() const override { return false; }

  void debug(const std::u16string& msg) override { push("h:debug=" + to_ascii(msg)); }

  bool survival_rank_mode() const override { return false; }
  bool survival_rank_available() const override { return false; }

  // TS 的 `World.handle_cmds` 直接 `CMDS.handle`；端口的这面是 `IWorldLfw::handle_cmds` 缝
  // ⇒ 假件把 `CMDS` 本体接上（不额外记日志，TS 那边没有这一层）。
  void handle_cmds(World& world) override { lfw::cmds::CMDS::handle(world, g_cmds); }

  void ctrl_come(lfw::controller::BaseController& ctrl, double x, double y, double z) override {
    (void)ctrl;
    push("h:come=" + num(x) + "," + num(y) + "," + num(z));
  }

  void ctrl_move(lfw::controller::BaseController& ctrl) override {
    (void)ctrl;
    push("h:movectl");
  }

  void ctrl_stay(lfw::controller::BaseController& ctrl) override {
    (void)ctrl;
    push("h:stay");
  }

  void ctrl_follow(lfw::controller::BaseController& ctrl, lfw::Entity& target) override {
    (void)ctrl;
    (void)target;
    push("h:follow");
  }

  bool ctrl_goingto(const lfw::controller::BaseController& ctrl) const override {
    (void)ctrl;
    return false;
  }

  void warn(const std::u16string& text) override { push("h:warn=" + esc(text)); }
};

FakeLfw g_lfw;

// ---------------------------------------------------------------- probe

// 两侧同名命令：把解析层的结果逐条打出来（键名表也必须一致）。
const char16_t* const kProbeNames[] = {u"--a", u"--a=", u"x", u"--", u"-", u"--eq", u"=y"};

std::string render_nums(const std::vector<double>& nums) {
  auto arr = std::make_shared<lfw::Array>();
  for (const double d : nums) arr->push_back(Value(d));
  return vstr(Value(arr));
}

void probe_cmd(lfw::cmds::CMDS& ctx) {
  push("p|" + esc(ctx.cmd()));
  for (size_t i = 0; i < ctx.words().size(); ++i) push("pw|" + std::to_string(i) + "|" + esc(ctx.words()[i]));
  for (size_t i = 0; i < ctx.positionals().size(); ++i) {
    push("pp|" + std::to_string(i) + "|" + esc(ctx.positionals()[i]));
  }
  for (size_t i = 0; i < 6; ++i) {
    const std::optional<std::u16string> s = ctx.str(i);
    push("ps|" + std::to_string(i) + "|" + (s.has_value() ? esc(*s) : std::string("u")));
    const std::optional<double> d = ctx.num(i);
    push("pn|" + std::to_string(i) + "|" + (d.has_value() ? vstr(Value(*d)) : std::string("u")));
    const std::optional<std::vector<double>> ns = ctx.nums(i);
    push("pns|" + std::to_string(i) + "|" + (ns.has_value() ? render_nums(*ns) : std::string("u")));
  }
  for (const char16_t* const name : kProbeNames) {
    const std::u16string key(name);
    const std::optional<std::u16string> s = ctx.str_arg(key);
    const std::optional<double> d = ctx.num_arg(key);
    const std::optional<std::vector<double>> ns = ctx.nums_arg(key);
    push("pa|" + esc(key) + "|" + (s.has_value() ? esc(*s) : std::string("u")) + "|" +
         (d.has_value() ? vstr(Value(*d)) : std::string("u")) + "|" +
         (ns.has_value() ? render_nums(*ns) : std::string("u")));
  }
}

// ---------------------------------------------------------------- 摘要

std::string dump_entity(const lfw::Entity& e) {
  return to_ascii(e.id) + ":" + num(e.hp()) + ":" + num(e.hp_r()) + ":" + num(e.mp()) + ":" +
         esc(e.team()) + ":" + flag(lfw::entity::is_fighter(lfw::ref_of(e))) + ":" +
         flag(lfw::entity::is_weapon(lfw::ref_of(e))) + ":" + flag(e.puppet) +
         ":fr=" + vstr(lfw::field_or(e.frame, u"id")) +
         ":pos=" + num(e.position.x) + "," + num(e.position.y) + "," + num(e.position.z) +
         ":fc=" + num(e.facing) + ":nm=" + vstr(e.name()) +
         ":did=" + vstr(lfw::field_or(e.data(), u"id"));
}

std::string dump_list(const std::vector<lfw::Entity*>& list) {
  std::string out;
  for (const lfw::Entity* const e : list) {
    if (e == nullptr) continue;
    if (!out.empty()) out += ";";
    out += dump_entity(*e);
  }
  return out.empty() ? "-" : out;
}

std::string vec2_or_z(const lfw::Vector2* v) {
  return v == nullptr ? std::string("z") : num(v->x) + "," + num(v->y);
}

std::string dump() {
  World& w = *g_world;
  std::string counts;
  for (const std::pair<std::u16string, double>& kv : w.counts()) {
    if (!counts.empty()) counts += ",";
    counts += esc(kv.first) + ":" + num(kv.second);
  }
  std::string pups;
  for (const std::pair<std::u16string, lfw::Entity*>& kv : w.puppets) {
    if (!pups.empty()) pups += ",";
    pups += to_ascii(kv.first) + ":" + (kv.second != nullptr ? to_ascii(kv.second->id) : std::string("z"));
  }
  return "dump|paused=" + num(w.paused_value()) + "|fn=" + num(w.fn_locked_value()) +
         "|diff=" + vstr(w.dataset.get(u"difficulty")) +
         "|playrate=" + vstr(w.dataset.get(u"playrate")) +
         "|inf=" + vstr(w.dataset.get(u"infinity_mp")) +
         "|cam=" + num(w.camera().position.x) + "," + num(w.camera().position.y) +
         "|lock=" + vec2_or_z(w.camera().locked()) + "|dest=" + vec2_or_z(w.camera().dested()) +
         "|lim=" + flag(w.stage_limit()) + "|st=" + vstr(Value(w.stage()->id())) +
         "|bg=" + vstr(Value(w.bg()->id())) +
         "|cnt=" + (counts.empty() ? std::string("-") : counts) +
         "|pup=" + (pups.empty() ? std::string("-") : pups) +
         "|ents=" + dump_list(w.entities) + "|ghosts=" + dump_list(w.ghosts) +
         "|cmds=" + std::to_string(g_cmds.size());
}

void emit_all() {
  for (const std::string& line : g_out) std::printf("%s\n", line.c_str());
  g_out.clear();
}

std::vector<std::pair<std::string, std::unique_ptr<lfw::Entity>>> g_entities;

lfw::Entity* g_entity_of_label(const std::string& label) {
  for (const std::pair<std::string, std::unique_ptr<lfw::Entity>>& kv : g_entities) {
    if (kv.first == label) return kv.second.get();
  }
  return nullptr;
}

double number_arg(const std::vector<std::string>& t, size_t& i, const std::string& op, int lineno) {
  const Value v = parse_value(t, i);
  const double* const d = std::get_if<double>(&v);
  if (d == nullptr) {
    std::fprintf(stderr, "%s expects a number literal at line %d\n", op.c_str(), lineno);
    std::exit(2);
  }
  return *d;
}

}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: lfw_trace_cmds.exe <case-file>\n");
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", argv[1]);
    return 2;
  }

  // `Stage` 的两参告警（`Ditto.warn`）与 `IWorldLfw::warn` 分开记，TS 侧同款。
  lfw::stage::Stage::set_warn([](const std::u16string& where, const std::u16string& text) {
    push("warn:" + to_ascii(where) + ":" + to_ascii(text));
  });

  lfw::cmds::CMDS::register_cmd(u"__PROBE__", u"", probe_cmd);

  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::vector<std::string> t = split_ws(trace::strip_comment(raw));
    if (t.empty()) continue;
    const std::string& op = t[0];
    size_t i = 1;

    if (op == "wnew") {
      g_world = std::make_unique<World>(g_lfw, g_renderer, &g_states);
      push("new|bg=" + vstr(Value(g_world->bg()->id())) + "|st=" + vstr(Value(g_world->stage()->id())));
    } else if (op == "hdump") {
      push(dump());
    } else if (op == "ds") {
      const std::u16string key = trace::key_of(t[i++]);
      g_world->dataset.set(key, parse_value(t, i));
    } else if (op == "bdata") {
      g_bg_datas.push_back(parse_value(t, i));
    } else if (op == "sdata") {
      g_stage_datas.push_back(parse_value(t, i));
    } else if (op == "data") {
      g_datas.push_back(parse_value(t, i));
    } else if (op == "fdata") {
      g_fighter_datas.push_back(parse_value(t, i));
    } else if (op == "wdata") {
      g_weapon_datas.push_back(parse_value(t, i));
    } else if (op == "player") {
      const std::string pid = t[i++];
      const std::string pname = t[i++];
      auto p = std::make_unique<lfw::PlayerInfo>(&g_player_host, to_u16(pid), Value(to_u16(pname)),
                                                 Value(true), Value(true));
      g_player_map.emplace_back(pid, p.get());
      g_players.push_back(std::move(p));
    } else if (op == "bg") {
      g_world->change_bg(parse_value(t, i));
    } else if (op == "stage") {
      g_world->change_stage(parse_value(t, i));
    } else if (op == "cheat") {
      const std::string name = t[i++];
      const bool on = t[i++] == "1";
      std::vector<std::string> next;
      for (const std::string& n : g_cheats) {
        if (n != name) next.push_back(n);
      }
      if (on) next.push_back(name);
      g_cheats = next;
    } else if (op == "mk" || op == "add") {
      const std::string label = t[i++];
      auto e = std::make_unique<lfw::Entity>(g_world->host(), parse_value(t, i), &g_states);
      lfw::Entity* const raw_e = e.get();
      g_entities.emplace_back(label, std::move(e));
      if (op == "add") g_world->add_entities(*raw_e);
      push((op == "add" ? "add|" : "mk|") + label + "|id=" + to_ascii(raw_e->id));
    } else if (op == "ent") {
      lfw::Entity* const e = g_entity_of_label(t[i++]);
      if (e == nullptr) {
        std::fprintf(stderr, "line %d: no such entity\n", lineno);
        return 2;
      }
      const std::string field = t[i++];
      if (field == "hp") e->set_hp(number_arg(t, i, op, lineno));
      else if (field == "hpr") e->set_hp_r(number_arg(t, i, op, lineno));
      else if (field == "mp") e->set_mp(number_arg(t, i, op, lineno));
      else if (field == "team") e->set_team(trace::key_of(t[i++]));
      else {
        std::fprintf(stderr, "line %d: unknown entity field '%s'\n", lineno, field.c_str());
        return 2;
      }
      push("ent|" + to_ascii(e->id) + "|" + field);
    } else if (op == "pup") {
      const std::u16string pid = trace::key_of(t[i++]);
      lfw::Entity* const e = g_entity_of_label(t[i++]);
      if (e == nullptr) {
        std::fprintf(stderr, "line %d: no such entity\n", lineno);
        return 2;
      }
      g_world->puppets.emplace_back(pid, e);
      push("pup|" + to_ascii(pid) + "|" + to_ascii(e->id));
    } else if (op == "ctrl") {
      lfw::Entity* const e = g_entity_of_label(t[i++]);
      if (e == nullptr) {
        std::fprintf(stderr, "line %d: no such entity\n", lineno);
        return 2;
      }
      const std::string kind = t[i++];
      lfw::controller::BaseController* const c = make_ctrl(kind);
      if (c != nullptr && i < t.size()) c->player_id = trace::key_of(t[i++]);
      e->set_ctrl(c);
    } else if (op == "cmd") {
      const Value v = parse_value(t, i);
      const std::u16string* const s = std::get_if<std::u16string>(&v);
      std::vector<std::u16string> list;
      list.push_back(s != nullptr ? *s : std::u16string());
      lfw::cmds::CMDS::handle(*g_world, list);
    } else if (op == "wcmds") {
      g_cmds.clear();
      while (i < t.size()) {
        const Value v = parse_value(t, i);
        const std::u16string* const s = std::get_if<std::u16string>(&v);
        g_cmds.push_back(s != nullptr ? *s : std::u16string());
      }
      push("wcmds|" + std::to_string(g_cmds.size()));
    } else if (op == "handlecmds") {
      g_world->handle_cmds();
    } else if (op == "h") {
      const Value v = parse_value(t, i);
      const std::u16string* const s = std::get_if<std::u16string>(&v);
      const std::u16string key = s != nullptr ? *s : std::u16string();
      push("h|" + esc(key) + "|" + flag(lfw::cmds::CMDS::handler(key) != nullptr));
    } else {
      std::fprintf(stderr, "line %d: unknown op '%s'\n", lineno, op.c_str());
      return 2;
    }
  }

  emit_all();
  return 0;
}
