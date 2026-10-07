#pragma once

#include "lfw/cmds/cmds.h"

namespace lfw {
namespace cmds {

// `CMD_DIST_CAM` / `CMD_LOCK_CAM`（数字解析走 `ctx.nums(1)`）。
const char16_t* cmd_dist_cam_help();
void cmd_dist_cam(CMDS& ctx);
const char16_t* cmd_lock_cam_help();
void cmd_lock_cam(CMDS& ctx);

}
}
