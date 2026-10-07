#include "lfw/cmds/cmd_set_puppet.h"

#include <optional>
#include <string>

#include "lfw/bg/background.h"
#include "lfw/controller/base_controller.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/player_info.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

const char16_t* cmd_set_puppet_help() {
  return uR"(Usage: SET_PUPPET --player_id=<id> --oid=<fighter> [--team=<team>]

Create or update a player's puppet / 创建或更新指定玩家的傀儡

Options:
  --player_id=<id>    player id (required) / 玩家 id（必填）
  --oid=<fighter>     fighter oid (required) / 角色 oid（必填）
  --team=<team>       team / 队伍)";
}

// TS 逐条对照：
//   * 缺参判定 `!player_id || !oid`（空串也算缺）；
//   * `is_human_ctrl(f.ctrl)` 读 `ctrl.__is_human_ctrl__` ⇒ 端口读 `ctrl->is_human()`；
//   * `f.ctrl.player_id != player_id` 两个字符串的宽松比较（与严格一致）；
//   * `f.data !== data` 是**引用**比较（`Value` 的深比较不行 ⇒ 比 `Object` 身份）。
void cmd_set_puppet(CMDS& ctx) {
  World& world = ctx.world();
  const std::optional<std::u16string> player_id = ctx.str_arg(u"--player_id");
  const std::optional<std::u16string> oid = ctx.str_arg(u"--oid");
  const std::optional<std::u16string> team = ctx.str_arg(u"--team");
  if (!player_id.has_value() || player_id->empty() || !oid.has_value() || oid->empty()) {
    world.lfw().warn(
        u"SET_PUPPET failed, must \"SET_PUPPET --player_id=1 --oid=deep [--team=...]\", got: " +
        ctx.cmd());
    return;
  }
  PlayerInfo* const player_info = world.lfw().player(Value(*player_id));
  if (player_info == nullptr) {
    world.lfw().warn(u"SET_PUPPET failed, player not found: " + *player_id);
    return;
  }
  const Value data = world.lfw().datas_fighters_find(Value(*oid));
  if (!truthy(data)) {
    world.lfw().warn(u"SET_PUPPET failed, fighter oid not found: " + *oid);
    return;
  }
  Entity* f = nullptr;
  for (const std::pair<std::u16string, Entity*>& kv : world.puppets) {
    if (kv.first == *player_id) f = kv.second;
  }
  if (f == nullptr) {
    f = world.lfw().create_entity(world, data);
    if (f == nullptr) {
      world.lfw().warn(u"SET_PUPPET failed to create puppet: " + *oid);
      return;
    }
    const Background::Middle& middle = world.middle();
    f->set_position(Value(middle.x), Value(450.0), Value(middle.z));
  }
  f->set_name(player_info->name());
  if (team.has_value() && !team->empty()) f->set_team(*team);
  if (as_object(f->data()) != as_object(data)) f->transform(data);
  controller::BaseController* const ctrl = f->ctrl();
  if (ctrl == nullptr || !ctrl->is_human() || ctrl->player_id != *player_id) {
    f->set_ctrl(world.lfw().acquire_local_ctrl(*player_id, *f));
  }
  f->attach();
}

}
}
