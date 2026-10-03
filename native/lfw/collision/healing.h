#pragma once

#include <functional>
#include <string>

#include "lfw/buff/buff.h"
#include "lfw/collision/collision.h"
#include "lfw/core/value.h"

namespace lfw {
namespace collision {

struct IHealingEntity {
  virtual ~IHealingEntity() = default;
  virtual const std::u16string& id() const = 0;
  virtual Value dataset(const std::u16string& key) const = 0;
  virtual buff::IBuffEntity* buff_entity() = 0;
};

struct HealingEnv {
  std::function<IHealingEntity*(const std::u16string& id)> find_entity;
  std::function<const buff::BuffEnv*()> buff_env;
};

const HealingEnv& healing_env();
void set_healing_env(const HealingEnv& env);

void handle_healing(Collision& c);

}
}
