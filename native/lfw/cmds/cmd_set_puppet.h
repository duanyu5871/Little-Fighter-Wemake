#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_SET_PUPPET`。
const char16_t* cmd_set_puppet_help();
void cmd_set_puppet(CMDS& ctx);

}
}
