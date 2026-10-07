#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_F1` / `CMD_F2` / `CMD_F3` / `CMD_F5` / `CMD_F6` / `CMD_F7` / `CMD_F8` /
// `CMD_F9` / `CMD_F10`（同名前缀的 `CMD_F4` 未移植：要 UI 层）。
const char16_t* cmd_f1_help();
void cmd_f1(CMDS& ctx);
const char16_t* cmd_f2_help();
void cmd_f2(CMDS& ctx);
const char16_t* cmd_f3_help();
void cmd_f3(CMDS& ctx);
const char16_t* cmd_f5_help();
void cmd_f5(CMDS& ctx);
const char16_t* cmd_f6_help();
void cmd_f6(CMDS& ctx);
const char16_t* cmd_f7_help();
void cmd_f7(CMDS& ctx);
const char16_t* cmd_f8_help();
void cmd_f8(CMDS& ctx);
const char16_t* cmd_f9_help();
void cmd_f9(CMDS& ctx);
const char16_t* cmd_f10_help();
void cmd_f10(CMDS& ctx);

}
}
