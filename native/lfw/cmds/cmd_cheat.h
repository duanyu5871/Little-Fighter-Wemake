#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_GIM_INK` / `CMD_HERO_FT` / `CMD_LF2_NET`（三条都登记到 `cheat_code_handler`）。
const char16_t* cmd_gim_ink_help();
const char16_t* cmd_hero_ft_help();
const char16_t* cmd_lf2_net_help();

}
}
