#include "lfw/state/weapon_state_base.h"

#include <memory>
#include <string>
#include <variant>

#include "lfw/defines/frame_id.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/weapon_bounce.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"

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

bool nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

Value coalesce2(const Value& a, const Value& b, const Value& fallback) {
  if (!nullish(a)) return a;
  if (!nullish(b)) return b;
  return fallback;
}

Value next_frame(const Value& id) {
  Object o;
  o.set(u"id", id);
  return Value(std::make_shared<Object>(o));
}

Value wsb_get_auto_frame(IStateEntity& e) {
  if (!e.has_data_indexes()) return Value();
  if (e.is_on_ground()) return index_by(e.data_frames(), to_string(e.data_indexes_on_ground()));
  return index_by(e.data_frames(), to_string(index_0(e.data_indexes_in_the_skys())));
}

void wsb_on_landing(IStateEntity& e, const Value& velocity) {
  (void)velocity;
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  e.enter_frame_by_id(to_string(e.data_indexes_on_ground()));
}

void wsb_on_leave_ground(IStateEntity& e) {
  e.enter_frame(next_frame(Value(std::u16string(frame_id::kAuto))));
}

}

WeaponState_Base::WeaponState_Base(Value state) : State_Base(std::move(state)) {
  get_auto_frame = &wsb_get_auto_frame;
  on_landing = &wsb_on_landing;
  on_leave_ground = &wsb_on_leave_ground;
}

void WeaponState_Base::update(IStateEntity& e) {
  e.handle_ground_velocity_decay();
}

void WeaponState_Base::hit_ground_rebouncing(IStateEntity& e, const Value& nf,
                                             const Value& velocity) {
  const double vy = to_number(field_or(velocity, u"y"));
  const double vx = to_number(field_or(velocity, u"x"));
  const double vz = to_number(field_or(velocity, u"z"));
  const Value base = e.data_base();
  const Value wt = e.base_type();

  const Value bounce_x =
      coalesce2(field_or(base, u"bounce_x"), wt_bounce_x(wt), Value(0.5));
  const Value bounce_y =
      coalesce2(field_or(base, u"bounce_y"), wt_bounce_y(wt), Value(0.5));
  const Value bounce_z =
      coalesce2(field_or(base, u"bounce_z"), wt_bounce_z(wt), Value(0.5));

  const Value bounce_min_x =
      coalesce2(field_or(base, u"bounce_min_x"), wt_bounce_min_x(wt), Value(99.0));
  const Value bounce_min_y =
      coalesce2(field_or(base, u"bounce_min_y"), wt_bounce_min_y(wt), Value(0.5));
  const Value bounce_min_z =
      coalesce2(field_or(base, u"bounce_min_z"), wt_bounce_min_z(wt), Value(99.0));

  const Value fast_x = coalesce2(field_or(base, u"fast_vx"), wt_fast_x(wt), Value(99.0));
  const Value fast_y = coalesce2(field_or(base, u"fast_vy"), wt_fast_y(wt), Value(99.0));
  const Value fast_z = coalesce2(field_or(base, u"fast_vz"), wt_fast_z(wt), Value(99.0));

  const double dvy = round_float(-vy * to_number(bounce_y));
  const double dvx = round_float(vx * to_number(bounce_x));
  const double dvz = round_float(vz * to_number(bounce_z));

  if (!truthy(e.drop_hurted())) {
    e.set_drop_hurted(Value(true));
    if (truthy(field_or(base, u"drop_hurt"))) {
      const Value hurt = field_or(base, u"drop_hurt");
      e.set_hp(Value(to_number(e.hp()) - to_number(hurt)));
      e.set_hp_r(Value(to_number(e.hp_r()) - to_number(hurt)));
    }
  }
  const bool is_bounce =
      dvy >= to_number(bounce_min_y) ||
      dvx >= to_number(bounce_min_x) || dvx < -to_number(bounce_min_x) ||
      dvx >= to_number(bounce_min_z) || dvx < -to_number(bounce_min_z);

  if (!is_bounce) {
    e.enter_frame_by_id(to_string(nf));
    return;
  }

  e.set_velocity(Value(dvx), Value(dvy), Value(dvz));
  e.leave_ground();
  if (equals(e.state(), Value(static_cast<double>(StateEnum::Weapon_Throwing))) &&
      dvy > -to_number(fast_y) && dvy < to_number(fast_y) &&
      dvx > -to_number(fast_x) && dvx < to_number(fast_x) &&
      dvz > -to_number(fast_z) && dvz < to_number(fast_z)) {
    const Value align = e.find_align_frame(e.frame_id(), e.data_indexes_throwings(),
                                          e.data_indexes_in_the_skys());
    if (truthy(align)) e.enter_frame(align);
  }
}

}
}
