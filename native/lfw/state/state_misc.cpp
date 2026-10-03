#include "lfw/state/state_misc.h"

#include <variant>

#include "lfw/defines/entity_enum.h"
#include "lfw/defines/gone_frame_info.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace state {
namespace {

void swb_on_landing(IStateEntity& e, const Value& velocity) {
  (void)velocity;
  e.enter_frame(gone_frame_info());
}

void cs2_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  const Value d = e.datas_find_fighter(u"50");
  if (!truthy(d)) return;
  e.transform(d);
  e.enter_frame(e.find_auto_frame());
}

void bsb_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  const Value st = e.state();
  if (!strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hitting))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Hit))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Rebounding))) &&
      !strict_equals(st, Value(static_cast<double>(StateEnum::Ball_Disappear)))) {
    return;
  }
  e.set_shaking(Value(0.0));
  e.set_motionless(Value(0.0));
  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));
}

}

State_WeaponBroken::State_WeaponBroken(Value state) : State_Base(std::move(state)) {
  on_landing = &swb_on_landing;
}

State_TransformToCatching::State_TransformToCatching(Value state) : State_Base(std::move(state)) {}

CharacterState_TransformToLouisEX::CharacterState_TransformToLouisEX(Value state)
    : State_Base(std::move(state)) {
  enter = &cs2_enter;
}

BallState_Base::BallState_Base(Value state) : State_Base(std::move(state)) {
  enter = &bsb_enter;
}

void State_TransformToCatching::update(IStateEntity& e) {
  e.transfrom_to_another();
  e.enter_frame(e.find_auto_frame());
}

void State_TransformTo8XXX::leave(IStateEntity& e, const Value& next_frame) {
  (void)next_frame;
  if (!std::holds_alternative<double>(state())) return;
  const std::u16string oid = to_string(Value(to_number(state()) - 8000));
  const Value data = e.datas_find(oid);
  const Value old_data = e.data();
  if (truthy(data)) e.transform(data);
  e.enter_frame(e.find_auto_frame());
  const Value new_type = e.data_type();
  if (!strict_equals(lfw::field_or(old_data, u"type"), new_type) &&
      strict_equals(new_type, Value(static_cast<double>(EntityEnum::Fighter)))) {
    e.world_callbacks_call(u"on_fighter_add");
  }
}

}
}
