#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_BGM` / `CMD_CHANGE_BG` / `CMD_CHANGE_STAGE` / `CMD_SET_DIFFICULTY`。
const char16_t* cmd_bgm_help();
void cmd_bgm(CMDS& ctx);
const char16_t* cmd_change_bg_help();
void cmd_change_bg(CMDS& ctx);
const char16_t* cmd_change_stage_help();
void cmd_change_stage(CMDS& ctx);
const char16_t* cmd_set_difficulty_help();
void cmd_set_difficulty(CMDS& ctx);

}
}
