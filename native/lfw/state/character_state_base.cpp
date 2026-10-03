#include "lfw/state/character_state_base.h"

#include <memory>
#include <string>

#include "lfw/defines/frame_id.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/weapon_type.h"

namespace lfw {
namespace state {
namespace {

Value index_by(const Value& holder, const std::u16string& k) {
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(k);
    if (p != nullptr) return *p;
  }
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    const double index = to_number(Value(k));
    if (!(index >= 0) || index >= static_cast<double>(arr->size())) return Value();
    return arr->at(static_cast<size_t>(index));
  }
  return Value();
}

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

Value next_frame(const Value& id) {
  Object o;
  o.set(u"id", id);
  return Value(std::make_shared<Object>(o));
}

void csb_on_landing(IStateEntity& e, const Value& velocity) {
  (void)velocity;
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  e.enter_frame_by_id(to_string(e.data_indexes_landing_2()));
}

Value csb_get_auto_frame(IStateEntity& e) {
  Value fid;
  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
    fid = e.data_indexes_heavy_obj_walk();
  } else if (e.is_on_ground()) {
    fid = e.data_indexes_default();
  } else if (to_number(e.hp()) > 0) {
    fid = index_0(e.data_indexes_in_the_skys());
  }
  if (!truthy(fid)) return Value();
  return index_by(e.data_frames(), to_string(fid));
}

Value csb_get_sudden_death_frame(IStateEntity& e) {
  e.set_velocity(Value(2 * to_number(e.facing())), Value(2.0), Value());
  const Value falling = e.data_indexes_falling();
  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"1"), u"1"));
  return Value();
}

Value csb_get_caught_end_frame(IStateEntity& e) {
  const Value cvx = e.dataset(u"cvx_d");
  const Value cvy = e.dataset(u"cvy_d");
  e.set_velocity(Value(-1 * to_number(cvx) * to_number(e.facing())), cvy, Value());
  const Value falling = e.data_indexes_falling();
  if (truthy(falling)) return next_frame(index_by(index_by(falling, u"-1"), u"1"));
  return Value();
}

void csb_on_leave_ground(IStateEntity& e) {
  const Value st = e.state();
  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Running))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Walking))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Standing))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Rowing)))) {
    return;
  }
  if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
    e.drop_holding();
  }
  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));
}

}

CharacterState_Base::CharacterState_Base(Value state) : State_Base(std::move(state)) {
  on_landing = &csb_on_landing;
  get_auto_frame = &csb_get_auto_frame;
  get_sudden_death_frame = &csb_get_sudden_death_frame;
  get_caught_end_frame = &csb_get_caught_end_frame;
  on_leave_ground = &csb_on_leave_ground;
}

void CharacterState_Base::update(IStateEntity& e) {
  State_Base::update(e);
  e.handle_ground_velocity_decay();
}

}
}
