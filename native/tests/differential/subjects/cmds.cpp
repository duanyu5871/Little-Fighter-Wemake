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
#include "world_fixture.h"

namespace {

using namespace fx;

using lfw::Value;
using lfw::World;
using trace::parse_value;
using trace::render_value;
using trace::split_ws;
using trace::to_ascii;
using trace::to_u16;

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
      auto e = std::make_unique<lfw::Entity>(*g_world, parse_value(t, i), &g_states);
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
