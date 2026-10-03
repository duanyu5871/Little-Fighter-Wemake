#include "lfw/state/character_state_caught_rowing.h"

#include "lfw/defines/speed_mode.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/entity/calc_v.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace state {
namespace {

void csc_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  e.set_fall_value(e.fall_value_max());
  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));
  const bool holding = e.has_holding();
  if (holding) e.drop_holding();
  if (holding &&
      strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
    e.holding_set_team(e.team());
  }
}

void csr_on_landing(IStateEntity& e, const Value& velocity) {
  (void)velocity;
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  e.enter_frame_by_id(to_string(e.data_indexes_landing_1()));
}

void csr_enter(IStateEntity& e, const Value& prev_frame) {
  if (!strict_equals(lfw::field_or(prev_frame, u"state"),
                     Value(static_cast<double>(StateEnum::Falling)))) {
    return;
  }
  const Value rowing_distance = e.dataset(u"rowing_distance");
  const Value bfall_x_f = e.dataset(u"bfall_x_f");
  const Value rowing_height = e.dataset(u"rowing_height");
  const Value bfall_h_f = e.dataset(u"bfall_h_f");
  const double vx = to_number(rowing_distance) * to_number(bfall_x_f);
  const double vy = to_number(rowing_height) * to_number(bfall_h_f);
  const Value prev_vx = e.velocity_x();
  const double prev_vy = e.velocity_y();
  const Value next_vx = to_number(prev_vx) >= 0 ? Value(vx) : Value(-vx);
  const double next_vy =
      entity::calc_v(prev_vy, vy, Value(static_cast<double>(SpeedMode::Default)), Value(0.0),
                     Value(1.0));
  e.set_velocity(next_vx, Value(next_vy), Value());
}

}

CharacterState_Caught::CharacterState_Caught(Value state) : CharacterState_Base(std::move(state)) {
  enter = &csc_enter;
}

void CharacterState_Caught::update(IStateEntity& e) {
  e.set_velocity(Value(0.0), Value(0.0), Value(0.0));
}

CharacterState_Rowing::CharacterState_Rowing(Value state) : CharacterState_Base(std::move(state)) {
  on_landing = &csr_on_landing;
  enter = &csr_enter;
}

}
}
