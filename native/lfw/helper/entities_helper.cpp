#include "lfw/helper/entities_helper.h"

#include <cmath>
#include <memory>
#include <utility>

#include "lfw/controller/base_controller.h"
#include "lfw/entity/entity.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace helper {

ObjectsHelper::ObjectsHelper(IHelperLfw& lfw)
    : _lfw(&lfw),
      _team_randoming(u"team_randoming",
                      {Value(std::u16string(u"1")), Value(std::u16string(u"2")),
                       Value(std::u16string(u"3")), Value(std::u16string(u"4"))},
                      &lfw.mt()) {}

std::vector<Entity*> ObjectsHelper::all() const {
  std::vector<Entity*> ret;
  for (Entity* const v : _lfw->world_entities()) ret.push_back(v);
  for (Entity* const v : _lfw->world_ghosts()) ret.push_back(v);
  return ret;
}

// `this.all[idx]`：JS 数组下标语义 —— 负数/越界/非整数都是 `undefined`（⇒ `nullptr`）。
Entity* ObjectsHelper::at(double idx) const {
  const std::vector<Entity*> list = all();
  if (!(idx >= 0)) return nullptr;
  if (std::floor(idx) != idx) return nullptr;
  if (idx >= static_cast<double>(list.size())) return nullptr;
  return list[static_cast<size_t>(idx)];
}

std::vector<Entity*> ObjectsHelper::add(const Value& data, double num, const std::u16string* team) {
  std::vector<Entity*> ret;
  // `while (--num >= 0)`：先减再比，`NaN` 直接不进循环。
  for (double n = num;;) {
    n -= 1;
    if (!(n >= 0)) break;
    Entity* const entity = _lfw->create_entity(data);
    if (entity == nullptr) continue;
    entity->set_ctrl(_lfw->create_ctrl(field_or(entity->data(), u"id"), std::u16string(), entity));
    // `team === '?' ? this.team_randoming.get() : (team || this.lfw.new_team)`
    if (team != nullptr && *team == u"?") {
      const Value picked = _team_randoming.get();
      const std::u16string* const s = std::get_if<std::u16string>(&picked);
      entity->set_team(s != nullptr ? *s : std::u16string());
    } else if (team != nullptr && !team->empty()) {
      entity->set_team(*team);
    } else {
      entity->set_team(_lfw->new_team());
    }
    _lfw->random_entity_info(*entity);
    entity->attach();
    ret.push_back(entity);
  }
  return ret;
}

void ObjectsHelper::del_all() { _lfw->del_entities(all()); }

}  // namespace helper
}  // namespace lfw
