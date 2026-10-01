#include "lfw/dat_translator/make_frame_state.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/opoint_kind.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/foreach.h"

namespace lfw {
namespace dat_translator {
namespace {

Value num(double v) { return Value(v); }

Value text(const char16_t* s) { return Value(std::u16string(s)); }

Value state_value(StateEnum s) { return Value(static_cast<double>(s)); }

bool is_state(const Value& state, StateEnum want) { return strict_equals(state, state_value(want)); }

Value make_obj(const std::vector<std::pair<const char16_t*, Value>>& kv) {
  Object o;
  for (const std::pair<const char16_t*, Value>& p : kv) o.set(std::u16string(p.first), p.second);
  return Value(std::make_shared<Object>(o));
}

Value action_auto(bool backward) {
  if (!backward) return make_obj({{u"id", text(u"auto")}});
  return make_obj({{u"id", text(u"auto")},
                   {u"facing", num(static_cast<double>(FacingFlag::Backward))}});
}

void each_object(Value& v, const std::function<void(Object&)>& fn) {
  foreach(v, [&fn](Value& item, size_t) {
    if (Object* o = as_object(item)) fn(*o);
  });
}

void fill_hit_flag(Value& v) {
  each_object(v, [](Object& o) { set_hit_flag(o, num(static_cast<double>(HitFlag::AllBoth))); });
}

void louis_cast_off(Value& frame, Object& o) {
  o.set(u"state", state_value(StateEnum::Frozen));
  const double dvy_a = 5;
  const double dvy_b = 4;
  const double dvx_b = 5.5;
  const double dvx_z = 4;
  const double y_b = 50;
  const double offset_z = 5;

  std::vector<Value> items;
  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39)},
                            {u"y", num(79)},
                            {u"oid", text(oid::kWeapon_LouisArmourB)},
                            {u"dvy", num(dvy_a)},
                            {u"action", action_auto(false)},
                            {u"speedz", num(0)}}));
  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39 + offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(dvx_z)},
                            {u"action", action_auto(true)},
                            {u"speedz", num(0)}}));
  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39 + offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(-30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(-dvx_z)},
                            {u"action", action_auto(true)},
                            {u"speedz", num(0)}}));
  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39 - offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(dvx_z)},
                            {u"action", action_auto(false)},
                            {u"speedz", num(0)}}));
  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39 - offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(-30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(-dvx_z)},
                            {u"action", action_auto(false)},
                            {u"speedz", num(0)}}));

  Value existing = o.get(u"opoint") != nullptr ? *o.get(u"opoint") : Value();
  o.set(u"opoint", ensure(existing, items));
}

void falling_bdy(Value& frame, Object& o) {
  const Value* bdyv = o.get(u"bdy");
  if (bdyv == nullptr) return;
  Value bdy = *bdyv;
  foreach(bdy, [](Value& item, size_t) {
    Object* b = as_object(item);
    if (b == nullptr) return;
    const Value* kind = b->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, num(static_cast<double>(BdyKind::Normal)))) return;
    CondMaker cm;
    cm.add(text(collision_val::kItrFall), u">=",
           num(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL")));
    cm.or_(text(collision_val::kItrKind), u"==",
           num(static_cast<double>(ItrKind::MagicFlute)));
    cm.or_(text(collision_val::kItrKind), u"==",
           num(static_cast<double>(ItrKind::MagicFlute2)));
    b->set(u"test", Value(cm.done()));
  });
}

}

void make_frame_state(Value& frame) {
  Object* o = as_object(frame);
  if (o == nullptr) return;
  const Value* statev = o->get(u"state");
  const Value state = statev != nullptr ? *statev : Value();

  if (is_state(state, StateEnum::Ball_3005)) {
    o->set(u"no_shadow", num(1));
  } else if (is_state(state, StateEnum::HeavyWeapon_OnHand) ||
             is_state(state, StateEnum::Weapon_OnHand)) {
    o->set(u"no_shadow", num(1));
    o->set(u"gravity_enabled", Value(false));
  } else if (is_state(state, StateEnum::Burning)) {
    if (const Value* itr = o->get(u"itr")) {
      Value arr = *itr;
      fill_hit_flag(arr);
    }
  } else if (is_state(state, StateEnum::OLD_LouisCastOff)) {
    louis_cast_off(frame, *o);
  } else if (is_state(state, StateEnum::Falling)) {
    falling_bdy(frame, *o);
  } else if (is_state(state, StateEnum::Frozen)) {
    if (const Value* bdy = o->get(u"bdy")) {
      Value arr = *bdy;
      fill_hit_flag(arr);
    }
  } else if (is_state(state, StateEnum::Message)) {
    o->set(u"no_shadow", num(1));
  }
}

}

}
