#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/helper/entities_helper.h"

namespace lfw {
namespace helper {

// TS `helper/WeaponsHelper.ts`。
class WeaponsHelper : public ObjectsHelper {
 public:
  using ObjectsHelper::ObjectsHelper;
  std::vector<Entity*> all() const override;
  // `add(data?: IEntityData | string, num = 1, team?)`
  std::vector<Entity*> add(const Value& data, double num = 1,
                           const std::u16string* team = nullptr) override;
  // `randoms(groups, duplicate)`：缓存键就是 `groups` 原文（含大小写/空白）。
  RandomingT<Value>* randoms(const std::u16string& groups, bool duplicate);
  // `add_random(num = 1, duplicate = false, group = '')`
  std::vector<Entity*> add_random(double num = 1, bool duplicate = false,
                                  const std::u16string& group = std::u16string());

 private:
  std::vector<std::pair<std::u16string, RandomingT<Value>::Ptr>> _random_map;
  std::vector<std::pair<std::u16string, RandomingT<Value>::Ptr>> _random_d_map;
};

}  // namespace helper
}  // namespace lfw
