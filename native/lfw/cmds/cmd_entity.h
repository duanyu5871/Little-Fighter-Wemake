#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_DESPAWN` / `CMD_DEL_PUPPET`。
const char16_t* cmd_despawn_help();
void cmd_despawn(CMDS& ctx);
const char16_t* cmd_del_puppet_help();
void cmd_del_puppet(CMDS& ctx);

}
}
