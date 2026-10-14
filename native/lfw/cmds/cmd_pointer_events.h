#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "lfw/cmds/cmds.h"

namespace lfw {

class World;

namespace cmds {

// TS `IPointerCursor`：某一个发起方（`--from`，本地是 `''`）的指针状态。
struct IPointerCursor {
  double x = 0;
  double y = 0;
  bool down = false;
  bool hidden = true;
  double t = 0;
};

const char16_t* cmd_pointer_down_help();
const char16_t* cmd_pointer_move_help();
const char16_t* cmd_pointer_up_help();
const char16_t* cmd_pointer_click_help();
const char16_t* cmd_pointer_cancel_help();
const char16_t* cmd_pointer_enter_help();
const char16_t* cmd_pointer_leave_help();

void cmd_pointer_down(CMDS& ctx);
void cmd_pointer_move(CMDS& ctx);
void cmd_pointer_up(CMDS& ctx);
void cmd_pointer_click(CMDS& ctx);
void cmd_pointer_cancel(CMDS& ctx);
void cmd_pointer_enter(CMDS& ctx);
void cmd_pointer_leave(CMDS& ctx);

// TS `export function get_pointer_cursors(world)`：给平台层画光标用；`nullptr` = 这个
// world 还没建过指针状态（TS 的 `undefined`）。返回的是**活表**（调用者可直接读）。
std::unordered_map<std::u16string, IPointerCursor>* get_pointer_cursors(World& world);
// TS `export function set_local_cursor(world, x, y, hidden, down)`：本地光标（`--from` 是
// `''`）的位置/隐藏/按下；`hidden`/`down` 省略时保留原值。
void set_local_cursor(World& world, double x, double y, std::optional<bool> hidden,
                      std::optional<bool> down);

}
}
