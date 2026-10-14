#include "lfw/cmds/cmd_key_event.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/controller/key_status.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/keys.h"
#include "lfw/lfw.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/ui_event.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

const char16_t* cmd_key_event_help() {
  return uR"(Usage: KEY_EVENT --p=<player_id> --s=<0|1> --c=<key_code> --n=<game_key>

Replay one local keyboard event: the UI layers see it first (from the top layer down, the first enabled one takes it), then every mounted key state gets it, and finally it reaches the given player's human controller (skipped while the stage disables control) / 重放一次本地键盘事件：先自顶向下派发给 UI 层（第一个未禁用的层生效），再派发给所有已挂载的按键状态，最后交给该玩家的真人控制器（关卡禁用操作时跳过）

Options:
  --p    player id (required) / 玩家 id（必填）
  --s    1 = pressed, anything else is a release / 1 为按下，其余值算抬起
  --c    lowercase "KeyboardEvent.key", e.g. "a" / "arrowleft" (required) / 键名，取 "KeyboardEvent.key" 并小写（必填）
  --n    game key, one of L/R/U/D/a/j/d (required) / 游戏键名，L/R/U/D/a/j/d 之一（必填）)";
}

void cmd_key_event(CMDS& ctx) {
  const std::optional<std::u16string> pid = ctx.str_arg(u"--p");
  const std::optional<double> state = ctx.num_arg(u"--s");
  const std::optional<std::u16string> key_code = ctx.str_arg(u"--c");
  const std::optional<std::u16string> key_name = ctx.str_arg(u"--n");

  if (!pid) {
    ctx.world().lfw().warn(u"KEY_EVENT failed, '--p' player id must be a string, got: " + ctx.cmd());
    return;
  }
  if (!key_code) {
    ctx.world().lfw().warn(u"KEY_EVENT failed, '--c' key code must be a string, got: " + ctx.cmd());
    return;
  }
  if (!key_name) {
    ctx.world().lfw().warn(u"KEY_EVENT failed, '--n' game key must be a string, got: " + ctx.cmd());
    return;
  }

  LFW& lfw = ctx.world().lfw();
  World& world = lfw.world();
  const bool pressed = state.has_value() && *state == 1.0;
  ui::LFWKeyEvent e(*pid, pressed, *key_name, *key_code);
  ui::UILayers* const layers = lfw.ui_layers();
  if (layers != nullptr) {
    for (int i = static_cast<int>(layers->length()) - 1; i >= 0; --i) {
      ui::UILayer* const layer = layers->at(static_cast<double>(i));
      ui::UINode* const ui = layer != nullptr ? layer->ui() : nullptr;
      if (ui == nullptr || ui->disabled()) continue;
      if (e.pressed) {
        ui->on_key_down(e);
      } else {
        ui->on_key_up(e);
      }
      break;
    }
  }

  const std::u16string& gk = e.game_key;
  for (Keys* const ks : lfw.mounted_keys) {
    controller::KeyStatus* const status = ks->get(gk);
    // TS 是 `ks[gk].hit()`：`gk` 不在表里会 `undefined.hit()` 抛；端口跳过（偏差见 DESIGN）。
    if (status == nullptr) continue;
    if (e.pressed) {
      status->hit(Value(), ks->time());
    } else {
      status->end(ks->time());
    }
  }

  if (world.stage()->control_disabled()) return;
  Entity* fighter = nullptr;
  for (const std::pair<std::u16string, Entity*>& item : world.puppets) {
    if (item.first == e.player) {
      fighter = item.second;
      break;
    }
  }
  if (fighter == nullptr) return;
  if (!entity::is_human_ctrl(fighter->ctrl_view())) return;

  const std::vector<std::u16string> keys{gk};
  if (e.pressed) {
    fighter->ctrl()->start(keys);
  } else {
    fighter->ctrl()->end(keys);
  }
}

}
}
