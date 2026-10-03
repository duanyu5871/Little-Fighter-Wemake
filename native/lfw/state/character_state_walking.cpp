#include "lfw/state/character_state_walking.h"

#include "lfw/defines/weapon_type.h"

namespace lfw {
namespace state {
namespace {

Value index_0(const Value& holder) {
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    if (arr->size() == 0) return Value();
    return arr->at(0);
  }
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(u"0");
    if (p != nullptr) return *p;
  }
  return Value();
}

}

void CharacterState_Walking::update(IStateEntity& e) {
  CharacterState_Base::update(e);
  if (!e.ctrl_ud() && !e.ctrl_lr() && !truthy(e.wait())) {
    if (e.holding_is_weapon() &&
        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
      e.set_wait(Value(e.handle_wait_flag(Value(), e.frame_info())));
    } else {
      e.enter_frame_by_id_fallback(to_string(e.data_indexes_default()), true);
    }
  }
  if (to_number(e.hp()) <= 0) {
    e.enter_frame(e.get_sudden_death_frame());
    return;
  }
  double px = 0;
  double py = 0;
  double pz = 0;
  e.position(px, py, pz);
  if (py > to_number(e.ground_y())) {
    e.enter_frame_by_id(to_string(index_0(e.data_indexes_in_the_skys())));
  }
}

}
}
