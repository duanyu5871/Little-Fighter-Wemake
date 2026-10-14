#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_KEY_EVENTS`。
const char16_t* cmd_key_event_help();
void cmd_key_event(CMDS& ctx);

}
}
