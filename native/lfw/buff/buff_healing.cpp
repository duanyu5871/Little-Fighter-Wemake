#include "lfw/buff/buff_healing.h"

#include "lfw/utils/math/base.h"

namespace lfw {
namespace buff {

const char16_t* Buff_Healing::KIND = u"Healing";

double Buff_Healing::duration_of(const IBuffEntity& e, double amount) {
  const double value = max(1.0, to_number(e.dataset(u"hp_healing_value")));
  const double ticks = max(1.0, to_number(e.dataset(u"hp_healing_ticks")));
  return ceil(amount / value) * ticks;
}

void Buff_Healing::mount() {
  Buff::mount();
  const std::vector<std::u16string>& victims = this->victims();
  for (size_t i = 0; i < victims.size(); ++i) {
    IBuffEntity* victim = env()->find_entity(victims[i]);
    if (victim == nullptr) continue;
    set_mark(*victim, std::u16string(KIND), id(), Value());
    set_ticks(to_number(victim->dataset(u"hp_healing_ticks")));
  }
}

void Buff_Healing::on_tick(IBuffEntity* attacker, IBuffEntity* victim) {
  (void)attacker;
  if (victim == nullptr) return;
  if (to_number(victim->hp()) >= to_number(victim->hp_r())) {
    set_lifetime(duration());
    return;
  }
  victim->set_hp(Value(min(to_number(victim->hp_r()),
                            to_number(victim->hp()) +
                                to_number(victim->dataset(u"hp_healing_value")))));
}

void Buff_Healing::unmount() {
  const std::vector<std::u16string>& victims = this->victims();
  for (size_t i = 0; i < victims.size(); ++i) {
    IBuffEntity* victim = env()->find_entity(victims[i]);
    if (victim == nullptr) continue;
    del_mark(*victim, std::u16string(KIND), Value(std::u16string(id())));
  }
  Buff::unmount();
}

}
}
