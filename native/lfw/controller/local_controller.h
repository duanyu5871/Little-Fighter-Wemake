#pragma once

#include "lfw/controller/base_controller.h"

namespace lfw {
namespace controller {

// `controller/LocalController.ts`：真人（键盘 / 手柄）控制器。TS 的两个额外动作在
// 端口里的落点：
//   * `readonly __is_human_ctrl__ = true` ⇒ `set_kind(true, false)`（`is_human()` 是
//     端口的标记读取面：`Entity::set_ctrl` 的 `mark_players_alive`、`ctrl_value()`
//     的 `__is_human_ctrl__` 字段都读它）；
//   * 构造与 `reset` 里各一次的 `this.player = this.lfw.player(player_id)`（父类也有
//     一遍）⇒ 切片 4CX 起由 `Factory::acquire_ctrl` 在造/复用控制器时绑定（见
//     `base_controller.h` 的 `player` 说明）。
class LocalController : public BaseController {
 public:
  static constexpr const char* TAG = "LocalController";

  LocalController() { set_kind(true, false); }
};

}  // namespace controller
}  // namespace lfw
