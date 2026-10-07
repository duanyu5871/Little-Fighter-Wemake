#include "lfw/cmds/cmd_entity.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/world.h"

namespace lfw {
namespace cmds {

const char16_t* cmd_despawn_help() {
  return uR"(Usage: DESPAWN <entity_id> ...

Remove entities directly by id / 按 id 直接移除实体)";
}

// TS 直接遍历 `c.words.slice(1)`（不是 positionals）：`--` 开头的词跳过。
void cmd_despawn(CMDS& ctx) {
  World& world = ctx.world();
  const std::vector<std::u16string>& words = ctx.words();
  for (size_t i = 1; i < words.size(); ++i) {
    const std::u16string& id = words[i];
    if (id.empty() || starts_with_dash(id)) continue;
    Entity* const e = world.find_entity(id);
    if (e == nullptr) continue;
    world.del_entity(*e);
  }
}

const char16_t* cmd_del_puppet_help() {
  return uR"(Usage: DEL_PUPPET <player_id>

Remove a player's puppet / 移除指定玩家的傀儡)";
}

void cmd_del_puppet(CMDS& ctx) {
  World& world = ctx.world();
  const std::optional<std::u16string> player_id = ctx.str(1);
  if (!player_id.has_value()) {
    world.lfw().warn(u"DEL_PUPPET failed, must \"DEL_PUPPET ${playerId}\", got: " + ctx.cmd());
    return;
  }
  Entity* entity = nullptr;
  for (const std::pair<std::u16string, Entity*>& kv : world.puppets) {
    if (kv.first == *player_id) entity = kv.second;
  }
  if (entity != nullptr) {
    world.del_entity(*entity);
  } else {
    world.lfw().warn(u"DEL_PUPPET failed, puppet not found.");
  }
}

}
}
