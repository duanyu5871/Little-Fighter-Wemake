#include "lfw/cmds/cmd_spawn.h"

#include <optional>
#include <string>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

const char16_t* cmd_spawn_help() {
  return uR"(Usage: SPAWN --oid=<oid> [options]

Spawn entity into the world / 生成实体到世界

Options:
  --oid=<oid>          data id (required) / 数据 oid（必填）
  --team=<team>        team / 队伍
  --name=<name>        display name / 显示名
  --player_id=<id>     control by the local player / 交给本地玩家控制
  --facing=<1|-1>      facing direction / 朝向
  --x, --y, --z=<n>    position, omit for random / 坐标，省略则随机落位
  --hp=<n>             hit points / 生命值
  --mp=<n>             mana points / 魔力值
  --count=<n>          amount to spawn, default 1 / 生成数量，默认 1)";
}

// TS 逐条对照：
//   * `--oid` 缺失或空串先告警；`datas.find` 拿不到数据再告警；
//   * `--count` 的 `?? 1` 只吞 `undefined`（`NaN` / 小数照进循环，`i < count` 的原样比较）；
//   * `team || lfw.new_team`（空串也算缺省，`new_team` 只在缺省那一支才读）；
//   * `facing === 1 || facing === -1`（严格等）；`x != void 0` 时 `y ?? 现值`；
//   * 返回的 `ids.join(',')` 没人接（`CMDS.handle` 丢返回值）⇒ 端口不拼字符串。
void cmd_spawn(CMDS& ctx) {
  World& world = ctx.world();
  const std::optional<std::u16string> oid = ctx.str_arg(u"--oid");
  const std::optional<std::u16string> team = ctx.str_arg(u"--team");
  const std::optional<std::u16string> name = ctx.str_arg(u"--name");
  const std::optional<std::u16string> player_id = ctx.str_arg(u"--player_id");
  const std::optional<double> facing = ctx.num_arg(u"--facing");
  const std::optional<double> x = ctx.num_arg(u"--x");
  const std::optional<double> y = ctx.num_arg(u"--y");
  const std::optional<double> z = ctx.num_arg(u"--z");
  const std::optional<double> hp = ctx.num_arg(u"--hp");
  const std::optional<double> mp = ctx.num_arg(u"--mp");
  const std::optional<double> count = ctx.num_arg(u"--count");
  const double count_value = count.has_value() ? *count : 1.0;
  if (!oid.has_value() || oid->empty()) {
    world.lfw().warn(u"SPAWN failed, must \"SPAWN --oid=xxx [--team=...] [--count=n]\", got: " +
                      ctx.cmd());
    return;
  }
  const Value data = world.lfw().datas_find(Value(*oid));
  if (!truthy(data)) {
    world.lfw().warn(u"SPAWN failed, oid not found: " + *oid);
    return;
  }
  const bool use_player = player_id.has_value() && !player_id->empty();
  for (double i = 0; i < count_value; i += 1) {
    Entity* const e = use_player ? world.lfw().create_entity_with_player(*player_id, world, data)
                                 : world.lfw().create_entity_with_bot(u"", world, data);
    if (e == nullptr) continue;
    e->set_team(team.has_value() && !team->empty() ? *team : world.lfw().new_team());
    if (name.has_value() && !name->empty()) e->set_name(Value(*name));
    if (facing.has_value() && (*facing == 1.0 || *facing == -1.0)) e->facing = *facing;
    if (x.has_value()) {
      e->position.set(*x, y.has_value() ? *y : e->position.y,
                      z.has_value() ? *z : e->position.z);
    } else {
      world.lfw().random_entity_info(*e);
    }
    if (hp.has_value()) e->set_hp(*hp);
    if (mp.has_value()) e->set_mp(*mp);
    e->attach();
  }
}

}
}
