#include "lfw/helper/characters_helper.h"

#include <memory>
#include <utility>
#include <vector>

#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"

namespace lfw {
namespace helper {

std::vector<Entity*> CharactersHelper::all() const {
  std::vector<Entity*> ret;
  for (Entity* const v : _lfw->world_entities()) {
    if (entity::is_fighter_data(v->data())) ret.push_back(v);
  }
  for (Entity* const v : _lfw->world_ghosts()) {
    if (entity::is_fighter_data(v->data())) ret.push_back(v);
  }
  return ret;
}

std::vector<Entity*> CharactersHelper::add(const Value& data, double num,
                                           const std::u16string* team) {
  Value d = data;
  if (std::get_if<std::u16string>(&d) != nullptr) {
    const Value* const found = _lfw->find_fighter(d);
    if (found == nullptr) return {};
    d = *found;
  }
  if (!truthy(d)) return {};
  return ObjectsHelper::add(d, num, team);
}

// `lfw.mt.pick(lfw.datas.fighters.filter(...))`：列表每次现筛（`pick` 只在数组分支抽下标）。
std::vector<Entity*> CharactersHelper::add_random(double num, const std::u16string* team,
                                                  const std::function<bool(const Value&)>* filter) {
  std::vector<Entity*> ret;
  for (double n = num;;) {
    n -= 1;
    if (!(n >= 0)) break;
    std::vector<Value> items;
    for (const Value& v : _lfw->fighters()) {
      if (filter == nullptr || (*filter)(v)) items.push_back(v);
    }
    const Value d = _lfw->mt().pick_value(Value(std::make_shared<Array>(std::move(items))));
    if (!truthy(d)) continue;
    const std::vector<Entity*> added = add(d, 1, team);
    ret.insert(ret.end(), added.begin(), added.end());
  }
  return ret;
}

}  // namespace helper
}  // namespace lfw
