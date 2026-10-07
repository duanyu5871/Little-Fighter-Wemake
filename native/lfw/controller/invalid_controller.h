#pragma once

#include "lfw/controller/base_controller.h"

namespace lfw {
namespace controller {

// `controller/InvalidController.ts`：占位控制器（`Entity` 出生 / 回收时挂它，
// `world.lfw.factory.acquire_ctrl(InvalidController, "", this)`）。它除了标记以外
// 什么也不做 —— 端口的标记读法同 `is_ball_ctrl()`。
class InvalidController : public BaseController {
 public:
  static constexpr const char* TAG = "InvalidController";

  bool is_invalid_controller() const override { return true; }
};

}  // namespace controller
}  // namespace lfw
