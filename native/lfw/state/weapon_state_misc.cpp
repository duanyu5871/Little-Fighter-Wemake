#include "lfw/state/weapon_state_misc.h"

#include <variant>

#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/weapon_bounce.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace state {
namespace {

bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

Value coalesce2(const Value& a, const Value& b, const Value& fallback) {
  if (!nullish(a)) return a;
  if (!nullish(b)) return b;
  return fallback;
}

bool is_boomerang(IStateEntity& e) {
  return equals(e.frame_behavior(), Value(static_cast<double>(FrameBehavior::Boomerang)));
}

void wsog_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  e.set_team(e.lfw_new_team());
}

void wsoh_pre_update(IStateEntity& e) {
  const Value motionless = e.motionless();
  if (!truthy(motionless) || !e.has_bearer()) return;
  e.set_bearer_motionless(Value(max(to_number(motionless), to_number(e.bearer_motionless()))));
}

Value wst_get_gravity(IStateEntity& e) {
  if (is_boomerang(e)) {
    return Value(round_float(to_number(e.dataset(u"weapon_throwing_gravity")) / 4.0));
  }
  return e.dataset(u"weapon_throwing_gravity");
}

void wst_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  e.leave_ground();
  e.set_drop_hurted(Value(false));
  if (is_boomerang(e)) {
    e.set_velocity(Value(to_number(e.velocity_x()) * 0.6), Value(), Value());
  }
}

void wst_on_landing(WeaponState_Base& self, IStateEntity& e, const Value& velocity) {
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  // TS: indexes?.throw_on_ground || indexes?.just_on_ground
  const Value throw_on_ground = e.data_indexes_throw_on_ground();
  const Value just_on_ground = e.data_indexes_just_on_ground();
  self.hit_ground_rebouncing(e, truthy(throw_on_ground) ? throw_on_ground : just_on_ground,
                             velocity);
}

void wsis_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  e.set_drop_hurted(Value(false));
}

void wsis_on_landing(WeaponState_Base& self, IStateEntity& e, const Value& velocity) {
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  self.hit_ground_rebouncing(e, e.data_indexes_just_on_ground(), velocity);
}

}

WeaponState_OnGround::WeaponState_OnGround(Value state) : WeaponState_Base(std::move(state)) {
  enter = &wsog_enter;
}

void WeaponState_OnGround::update(IStateEntity& e) { e.handle_ground_velocity_decay(); }

WeaponState_OnHand::WeaponState_OnHand(Value state) : WeaponState_Base(std::move(state)) {
  pre_update = &wsoh_pre_update;
}

WeaponState_Throwing::WeaponState_Throwing(Value state) : WeaponState_Base(std::move(state)) {
  get_gravity = &wst_get_gravity;
  enter = &wst_enter;
  on_landing = [this](IStateEntity& e, const Value& velocity) {
    wst_on_landing(*this, e, velocity);
  };
}

void WeaponState_Throwing::update(IStateEntity& e) { e.handle_ground_velocity_decay(); }

WeaponState_InTheSky::WeaponState_InTheSky(Value state) : WeaponState_Base(std::move(state)) {
  enter = &wsis_enter;
  on_landing = [this](IStateEntity& e, const Value& velocity) {
    wsis_on_landing(*this, e, velocity);
  };
}

void WeaponState_InTheSky::update(IStateEntity& e) {
  e.handle_ground_velocity_decay();

  const double vy = e.velocity_y();
  const double vx = to_number(e.velocity_x());
  const double vz = to_number(e.velocity_z());
  const Value base = e.data_base();
  const Value wt = e.base_type();
  const Value fast_x = coalesce2(field_or(base, u"fast_vx"), wt_fast_x(wt), Value(99.0));
  const Value fast_y = coalesce2(field_or(base, u"fast_vy"), wt_fast_y(wt), Value(99.0));
  const Value fast_z = coalesce2(field_or(base, u"fast_vz"), wt_fast_z(wt), Value(99.0));

  // 速度太快的，变为throwing
  if (!equals(wt, Value(static_cast<double>(WeaponEnum::Heavy))) &&
      (vy < -to_number(fast_y) || vy > to_number(fast_y) ||
       vx < -to_number(fast_x) || vx > to_number(fast_x) ||
       vz < -to_number(fast_z) || vz > to_number(fast_z))) {
    const Value nf = e.find_align_frame(e.frame_id(), e.data_indexes_in_the_skys(),
                                        e.data_indexes_throwings());
    if (truthy(nf)) {
      e.set_dropping(false);
      e.enter_frame(nf);
    }
  }
}

}
}
