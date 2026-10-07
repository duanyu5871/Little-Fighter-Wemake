#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_KILL` / `CMD_KILL_BOSS` / `CMD_KILL_ENEMIES` / `CMD_KILL_OTHERS` / `CMD_KILL_SOLIDERS`
// （文件对应 TS 的同名五份，含 TS 的拼写 `KILL_SOLIDERS`）。`CMD_KILL_BOSS` 的
// `stage.kill_boss` 等四个方法都在 `stage/Stage` 里。
const char16_t* cmd_kill_help();
void cmd_kill(CMDS& ctx);
const char16_t* cmd_kill_boss_help();
void cmd_kill_boss(CMDS& ctx);
const char16_t* cmd_kill_enemies_help();
void cmd_kill_enemies(CMDS& ctx);
const char16_t* cmd_kill_others_help();
void cmd_kill_others(CMDS& ctx);
const char16_t* cmd_kill_soliders_help();
void cmd_kill_soliders(CMDS& ctx);

}
}
