#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_SPAWN`。
const char16_t* cmd_spawn_help();
void cmd_spawn(CMDS& ctx);

}
}
