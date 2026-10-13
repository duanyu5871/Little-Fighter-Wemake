#include "lfw/state/character_state_basic.h"

#include "lfw/defines/weapon_type.h"
#include "lfw/utils/math/base.h"
#include "lfw/entity/entity.h"

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

void csi_enter(Entity& e, const Value& prev_frame) {
  (void)prev_frame;
  if (!strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) return;
  e.drop_holding();
  e.holding_set_team(e.team());
}

Value nan_to_null(double v) { return std::isnan(v) ? Value() : Value(v); }

}

void CharacterState_Standing::update(Entity& e) {
  CharacterState_Base::update(e);
  if (e.hp() <= 0) {
    e.enter_frame(e.get_sudden_death_frame());
    return;
  }
  double px = 0;
  double py = 0;
  double pz = 0;
  px = e.position.x; py = e.position.y; pz = e.position.z;
  if (py > e.ground_y()) {
    e.enter_frame_by_id(to_string(index_0(e.data_indexes_of(u"in_the_skys"))));
  }
}

void CharacterState_Running::update(Entity& e) {
  CharacterState_Base::update(e);
  const Value vz = e.velocity_z();
  double vx = e.velocity_x();
  if (truthy(vz)) {
    const double dz = abs(to_number(vz) / 4);
    if (vx > dz) vx = vx - dz;
    if (vx < -dz) vx = vx + dz;
    e.set_velocity(nan_to_null(vx), Value(), Value());
  }
  if (e.hp() <= 0) e.enter_frame(e.get_sudden_death_frame());
}

CharacterState_Injured::CharacterState_Injured(Value state) : CharacterState_Base(std::move(state)) {
  enter = &csi_enter;
}

}
}
