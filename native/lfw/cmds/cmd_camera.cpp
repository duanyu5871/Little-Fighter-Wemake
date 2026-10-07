#include "lfw/cmds/cmd_camera.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "lfw/camera.h"
#include "lfw/core/value.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

namespace {

// TS 的 `const [x, y = 0] = nums`：只有一段时 `y` 缺省 0。
void unpack2(const std::vector<double>& nums, double& x, double& y) {
  x = nums[0];
  y = nums.size() > 1 ? nums[1] : 0.0;
}

}

const char16_t* cmd_dist_cam_help() {
  return uR"(Usage: DIST_CAM [<x>[,<y>]]

Move the camera (omit to reset) / 移动摄像机位置（省略则回到默认）)";
}

void cmd_dist_cam(CMDS& ctx) {
  World& world = ctx.world();
  const std::optional<std::vector<double>> nums = ctx.nums(1);
  if (!nums.has_value()) {
    world.camera().undest();
    return;
  }
  double x = 0;
  double y = 0;
  unpack2(*nums, x, y);
  if (std::isnan(x)) {
    world.lfw().warn(u"DIST_CAM failed, x got " + to_string(Value(x)) + u".");
    return;
  }
  if (std::isnan(y)) {
    world.lfw().warn(u"DIST_CAM failed, y got " + to_string(Value(y)) + u".");
    return;
  }
  world.camera().dest(x, y);
}

const char16_t* cmd_lock_cam_help() {
  return uR"(Usage: LOCK_CAM [<x>[,<y>]]

Lock the camera (omit to unlock) / 锁定摄像机位置（省略则解锁）)";
}

void cmd_lock_cam(CMDS& ctx) {
  World& world = ctx.world();
  const std::optional<std::vector<double>> nums = ctx.nums(1);
  if (!nums.has_value()) {
    world.camera().unlock();
    return;
  }
  double x = 0;
  double y = 0;
  unpack2(*nums, x, y);
  if (std::isnan(x)) {
    world.lfw().warn(u"LOCK_CAM failed, x got " + to_string(Value(x)) + u".");
    return;
  }
  if (std::isnan(y)) {
    world.lfw().warn(u"LOCK_CAM failed, y got " + to_string(Value(y)) + u".");
    return;
  }
  world.camera().lock(x, y);
}

}
}
