#include "lfw/state/character_state_drink.h"

#include <memory>
#include <string>

#include "lfw/defines/frame_id.h"
#include "lfw/entity/drink_info.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace state {
namespace {

Value next_frame(const Value& id) {
  Object o;
  o.set(u"id", id);
  return Value(std::make_shared<Object>(o));
}

}

CharacterState_Drink::CharacterState_Drink(Value state) : CharacterState_Base(std::move(state)) {}

void CharacterState_Drink::update(IStateEntity& e) {
  CharacterState_Base::update(e);
  if (!e.has_holding()) return;
  DrinkInfo* drink = e.holding_drink();
  if (drink == nullptr) return;

  const bool hp_h_empty = drink->hp_h_empty();
  const bool hp_r_empty = drink->hp_r_empty();
  const bool mp_h_empty = drink->mp_h_empty();
  if (!hp_h_empty && drink->hp_h_ticks().add()) {
    e.set_hp(Value(min(to_number(e.hp_max()),
                       to_number(e.hp()) + to_number(drink->hp_h_value()))));
    drink->set_hp_h(Value(to_number(drink->hp_h()) + to_number(drink->hp_h_value())));
  }
  if (!hp_r_empty && drink->hp_r_ticks().add()) {
    e.set_hp_r(Value(min(to_number(e.hp_max()),
                         to_number(e.hp_r()) + to_number(drink->hp_r_value()))));
    drink->set_hp_r(Value(to_number(drink->hp_r()) + to_number(drink->hp_r_value())));
  }
  if (!mp_h_empty && drink->mp_h_ticks().add()) {
    e.set_mp(Value(min(to_number(e.mp_max()),
                       to_number(e.mp()) + to_number(drink->mp_h_value()))));
    drink->set_mp_h(Value(to_number(drink->mp_h()) + to_number(drink->mp_h_value())));
  }
  if (hp_h_empty && hp_r_empty && mp_h_empty) {
    e.drop_holding();
    e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));
    e.holding_set_hp_r(Value(1.0));
    e.holding_set_hp(Value(1.0));
    const double vx = to_number(e.holding_mt_range(-6.0, 6.0)) / 2.0;
    e.holding_set_velocity(Value(vx), Value(6.0), Value(0.0));
  }
}

}
}
