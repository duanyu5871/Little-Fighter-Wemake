#include "lfw/collision/healing.h"

#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace collision {
namespace {

HealingEnv g_env;

}

const HealingEnv& healing_env() { return g_env; }

void set_healing_env(const HealingEnv& env) { g_env = env; }

void handle_healing(Collision& c) {
  const Value injury = field_or(c.itr, u"injury");
  if (!truthy(injury)) return;

  IHealingEntity* a = g_env.find_entity(c.aid);
  IHealingEntity* v = g_env.find_entity(c.vid);
  if (v == nullptr) return;

  const double value = max(1.0, to_number(v->dataset(u"hp_healing_value")));
  const double ticks = max(1.0, to_number(v->dataset(u"hp_healing_ticks")));
  const double duration = ceil(to_number(injury) / value) * ticks;

  buff::grant_buff(g_env.buff_env(), u"Healing", a == nullptr ? nullptr : a->buff_entity(),
                   v->buff_entity(), duration);
}

}
}
