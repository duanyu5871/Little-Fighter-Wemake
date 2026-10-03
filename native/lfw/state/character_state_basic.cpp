#include "lfw/state/character_state_basic.h"

#include "lfw/defines/weapon_type.h"
#include "lfw/utils/math/base.h"

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

void csi_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;
  e.drop_holding();
  e.holding_set_team(e.team());
}

}

void CharacterState_Standing::update(IStateEntity& e) {
  CharacterState_Base::update(e);
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

void CharacterState_Running::update(IStateEntity& e) {
  CharacterState_Base::update(e);
  const Value vz = e.velocity_z();
  Value vx = e.velocity_x();
  if (truthy(vz)) {
    const double dz = abs(to_number(vz) / 4);
    if (to_number(vx) > dz) vx = Value(to_number(vx) - dz);
    if (to_number(vx) < -dz) vx = Value(to_number(vx) + dz);
    e.set_velocity(vx, Value(), Value());
  }
  if (to_number(e.hp()) <= 0) e.enter_frame(e.get_sudden_death_frame());
}

CharacterState_Injured::CharacterState_Injured(Value state) : CharacterState_Base(std::move(state)) {
  enter = &csi_enter;
}

}
}
