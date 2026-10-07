#include "lfw/cmds/cmds.h"

#include <memory>
#include <utility>

#include "lfw/cmds/cheat_code_handler.h"
#include "lfw/cmds/cmd_camera.h"
#include "lfw/cmds/cmd_cheat.h"
#include "lfw/cmds/cmd_entity.h"
#include "lfw/cmds/cmd_f.h"
#include "lfw/cmds/cmd_kill.h"
#include "lfw/cmds/cmd_scene.h"
#include "lfw/cmds/cmd_set_puppet.h"
#include "lfw/cmds/cmd_spawn.h"
#include "lfw/core/js_string.h"
#include "lfw/defines/cmd.h"
#include "lfw/utils/string_help.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

namespace {

std::u16string lower(const std::u16string& s) { return to_lower_case(s); }

struct Registry {
  std::map<std::u16string, CmdHandler> handlers;
  std::map<std::u16string, std::u16string> helps;
  bool ready = false;
};

Registry& registry() {
  static Registry r;
  return r;
}

void put(const std::u16string& key, const char16_t* help, CmdHandler fn) {
  Registry& r = registry();
  const std::u16string k = lower(key);
  r.helps[k] = help;
  r.handlers[k] = fn;
}

// 默认命令表：次序照 `src/LFW/cmds/index.ts`（只含已移植的命令）。
void register_defaults() {
  put(cmd::kBGM, cmd_bgm_help(), cmd_bgm);
  put(cmd::kCHANGE_BG, cmd_change_bg_help(), cmd_change_bg);
  put(cmd::kCHANGE_STAGE, cmd_change_stage_help(), cmd_change_stage);
  put(cmd::kDEL_PUPPET, cmd_del_puppet_help(), cmd_del_puppet);
  put(cmd::kDESPAWN, cmd_despawn_help(), cmd_despawn);
  put(cmd::kDIST_CAM, cmd_dist_cam_help(), cmd_dist_cam);
  put(cmd::kF1, cmd_f1_help(), cmd_f1);
  put(cmd::kF2, cmd_f2_help(), cmd_f2);
  put(cmd::kF3, cmd_f3_help(), cmd_f3);
  put(cmd::kF5, cmd_f5_help(), cmd_f5);
  put(cmd::kF6, cmd_f6_help(), cmd_f6);
  put(cmd::kF7, cmd_f7_help(), cmd_f7);
  put(cmd::kF8, cmd_f8_help(), cmd_f8);
  put(cmd::kF9, cmd_f9_help(), cmd_f9);
  put(cmd::kF10, cmd_f10_help(), cmd_f10);
  put(cmd::kGIM_INK, cmd_gim_ink_help(), cheat_code_handler);
  put(cmd::kHERO_FT, cmd_hero_ft_help(), cheat_code_handler);
  put(cmd::kKILL, cmd_kill_help(), cmd_kill);
  put(cmd::kKILL_BOSS, cmd_kill_boss_help(), cmd_kill_boss);
  put(cmd::kKILL_ENEMIES, cmd_kill_enemies_help(), cmd_kill_enemies);
  put(cmd::kKILL_OTHERS, cmd_kill_others_help(), cmd_kill_others);
  put(cmd::kKILL_SOLIDERS, cmd_kill_soliders_help(), cmd_kill_soliders);
  put(cmd::kLF2_NET, cmd_lf2_net_help(), cheat_code_handler);
  put(cmd::kLOCK_CAM, cmd_lock_cam_help(), cmd_lock_cam);
  put(cmd::kSET_DIFFICULTY, cmd_set_difficulty_help(), cmd_set_difficulty);
  put(cmd::kSET_PUPPET, cmd_set_puppet_help(), cmd_set_puppet);
  put(cmd::kSPAWN, cmd_spawn_help(), cmd_spawn);
}

void ensure_ready() {
  Registry& r = registry();
  if (r.ready) return;
  r.ready = true;
  register_defaults();
}

}

CmdHandler CMDS::handler(const std::u16string& key) {
  ensure_ready();
  const std::map<std::u16string, CmdHandler>& table = registry().handlers;
  const std::map<std::u16string, CmdHandler>::const_iterator it = table.find(lower(key));
  return it == table.end() ? nullptr : it->second;
}

void CMDS::register_cmd(const std::u16string& key, const std::u16string& help, CmdHandler fn) {
  ensure_ready();
  Registry& r = registry();
  const std::u16string k = lower(key);
  r.helps[k] = help;
  r.handlers[k] = fn;
}

void CMDS::handle(World& world, const std::vector<std::u16string>& cmds) {
  static std::unique_ptr<CMDS> inst;
  if (inst == nullptr || inst->world_ != &world) inst.reset(new CMDS(world));
  for (size_t i = 0; i < cmds.size(); ++i) {
    inst->set_cmd(cmds[i]);
    // TS 的 `this.inst.words[0].toLowerCase()`；空白命令时 TS 会 TypeError，端口跳过。
    if (inst->words_.empty()) continue;
    const CmdHandler fn = CMDS::handler(inst->words_[0]);
    if (fn != nullptr) fn(*inst);
  }
}

CMDS::CMDS(World& world) : world_(&world) {}

void CMDS::set_cmd(const std::u16string& cmd) {
  cmd_ = cmd;
  words_.clear();
  positionals_.clear();
  size_t start = 0;
  for (size_t i = 0; i <= cmd.size(); ++i) {
    if (i != cmd.size() && cmd[i] != u' ') continue;
    const std::u16string part = js_trim(cmd.substr(start, i - start));
    start = i + 1;
    if (part.empty()) continue;
    words_.push_back(part);
    if (!starts_with_dash(part)) positionals_.push_back(part);
  }
  args_.reset();
}

std::optional<std::u16string> CMDS::str(size_t index) const {
  if (index >= positionals_.size()) return std::nullopt;
  return positionals_[index];
}

std::optional<double> CMDS::num(size_t index) const {
  const std::optional<std::u16string> s = str(index);
  if (!s.has_value()) return std::nullopt;
  return string_to_number(*s);
}

std::optional<std::vector<double>> CMDS::nums(size_t index) const {
  const std::optional<std::u16string> s = str(index);
  if (!s.has_value()) return std::nullopt;
  std::vector<double> out;
  size_t start = 0;
  for (size_t i = 0; i <= s->size(); ++i) {
    if (i != s->size() && (*s)[i] != u',') continue;
    out.push_back(string_to_number(s->substr(start, i - start)));
    start = i + 1;
  }
  return out;
}

const std::map<std::u16string, std::u16string>& CMDS::args() const {
  if (!args_.has_value()) {
    std::map<std::u16string, std::u16string> map;
    for (const std::u16string& token : words_) {
      const size_t eq = token.find(u'=');
      if (eq != std::u16string::npos && eq > 0) {
        map[token.substr(0, eq)] = token.substr(eq + 1);
      } else {
        map[token] = std::u16string();
      }
    }
    args_ = std::move(map);
  }
  return *args_;
}

std::optional<std::u16string> CMDS::arg_value(const std::u16string& name) const {
  const std::map<std::u16string, std::u16string>& a = args();
  const std::map<std::u16string, std::u16string>::const_iterator it = a.find(name);
  if (it == a.end()) return std::nullopt;
  return it->second;
}

std::optional<std::u16string> CMDS::str_arg(const std::u16string& name) const {
  return arg_value(name);
}

std::optional<double> CMDS::num_arg(const std::u16string& name) const {
  const std::optional<std::u16string> s = arg_value(name);
  if (!s.has_value()) return std::nullopt;
  return string_to_number(*s);
}

std::optional<std::vector<double>> CMDS::nums_arg(const std::u16string& name) const {
  const std::optional<std::u16string> s = arg_value(name);
  if (!s.has_value()) return std::nullopt;
  std::vector<double> out;
  size_t start = 0;
  for (size_t i = 0; i <= s->size(); ++i) {
    if (i != s->size() && (*s)[i] != u',') continue;
    out.push_back(string_to_number(s->substr(start, i - start)));
    start = i + 1;
  }
  return out;
}

}
}
