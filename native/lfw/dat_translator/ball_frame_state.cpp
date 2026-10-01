#include "lfw/dat_translator/ball_frame_state.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/ball_bdy.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/foreach.h"

namespace lfw {
namespace dat_translator {
namespace {

double itr_num(ItrKind k) { return static_cast<double>(k); }

double state_num(StateEnum s) { return static_cast<double>(s); }

double ent_num(EntityEnum e) { return static_cast<double>(e); }

double effect_num(ItrEffect e) { return static_cast<double>(e); }

Value field_or_any(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

Array* as_array_mut(const Value& v) { return const_cast<Array*>(lfw::as_array(v)); }

Object* as_object_mut(const Value& v) { return const_cast<Object*>(lfw::as_object(v)); }

Value make_empty_array() { return Value(std::make_shared<Array>(Array())); }

Value ensure_field_array(Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  if (v != nullptr && truthy(*v)) return *v;
  const Value arr = make_empty_array();
  o.set(std::u16string(key), arr);
  return arr;
}

Value make_bdy_ctx(const Object& base_ctx, const Value& bdy, double index) {
  Object o = base_ctx;
  o.set(u"bdy", bdy);
  o.set(u"index", n(index));
  return Value(std::make_shared<Object>(o));
}

Value action_v_next(const char16_t* id) {
  return make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                   {u"data", make_obj({{u"id", s(id)}})}});
}

void edit_bdy_hit_flag_all_both(Value& bdy) {
  Object fields;
  fields.set(u"hit_flag", n(static_cast<double>(HitFlag::AllBoth)));
  edit_bdy_edit(bdy, Value(std::make_shared<Object>(fields)));
}

bool is_bdy_normal(const Value& bdy) {
  const Object* bo = as_object(bdy);
  if (bo == nullptr) return false;
  const Value* kind = bo->get(u"kind");
  return kind != nullptr && equals(*kind, n(static_cast<double>(BdyKind::Normal)));
}

void cook_shrink_and_bounce(Value& ctx, Object& frame, bool with_itr) {
  const Value* data_ptr = nullptr;
  Object* c = as_object(ctx);
  if (c != nullptr) data_ptr = c->get(u"data");
  const Value data = data_ptr != nullptr ? *data_ptr : Value();
  const Object* dobj = as_object_mut(data);
  const Value* id_ptr = dobj != nullptr ? dobj->get(u"id") : nullptr;
  const Value id = id_ptr != nullptr ? *id_ptr : Value();

  const Value list = ensure_field_array(frame, u"bdy");
  Array* arr = as_array_mut(list);
  if (arr == nullptr) return;
  Array new_bdy;
  const size_t count = arr->size();
  for (size_t i = 0; i < count; ++i) {
    Value bdy = arr->at(i);
    if (!is_bdy_normal(bdy)) continue;
    if (equals(id, s(oid::kFreezeColumn))) edit_bdy_hit_flag_all_both(bdy);
    Value ctx20 = make_bdy_ctx(*c, bdy, static_cast<double>(i));
    arr->at(i) = cook_ball_bdy_get_hit_to_frame_20(ctx20);
    Value ctx30 = make_bdy_ctx(*c, bdy, -1);
    new_bdy.push_back(cook_ball_bdy_get_hit_to_frame_30(ctx30));
  }
  for (const Value& v : new_bdy.items()) arr->push_back(v);

  if (!with_itr) return;
  const Value itr_list = ensure_field_array(frame, u"itr");
  Array* itrs = as_array_mut(itr_list);
  if (itrs == nullptr) return;
  for (size_t i = 0; i < itrs->size(); ++i) {
    Object* io = as_object(itrs->at(i));
    if (io == nullptr) continue;
    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    Value actions = field_or_any(*io, u"actions");
    Object item;
    item.set(u"type", s(action_type::kA_NEXT_FRAME));
    item.set(u"data", make_obj({{u"id", s(u"10")}}));
    io->set(u"actions", ensure(actions, std::vector<Value>{Value(std::make_shared<Object>(item))}));
  }
}

}

void cook_ball_frame_state_15(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value frame = field_or_any(*c, u"frame");
  Object* f = as_object_mut(frame);
  if (f == nullptr) return;
  cook_shrink_and_bounce(ctx, *f, false);
}

void cook_ball_frame_state_3000(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value frame = field_or_any(*c, u"frame");
  Object* f = as_object_mut(frame);
  if (f == nullptr) return;
  cook_shrink_and_bounce(ctx, *f, true);
}

void cook_ball_frame_state_3001(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value frame = field_or_any(*c, u"frame");
  Object* f = as_object_mut(frame);
  if (f == nullptr) return;
  const Value data = field_or_any(*c, u"data");
  const Object* dobj = as_object_mut(data);
  const Value id = dobj != nullptr ? field_or_any(*dobj, u"id") : Value();

  const Value list = ensure_field_array(*f, u"bdy");
  Array* arr = as_array_mut(list);
  if (arr == nullptr) return;
  Array new_bdy;
  const size_t count = arr->size();
  for (size_t i = 0; i < count; ++i) {
    Value bdy = arr->at(i);
    CondMaker cond;
    cond.add(s(collision_val::kItrKind), u"!=", n(itr_num(ItrKind::JohnShield)));
    cond.and_(s(collision_val::kItrKind), u"!=", n(itr_num(ItrKind::Block)));
    cond.and_([&](CondMaker& cc) {
      cc.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));
      cc.or_([&](CondMaker& c2) {
        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Weapon)));
        c2.and_(s(collision_val::kAttackerState), u"!=", n(state_num(StateEnum::Weapon_OnHand)));
        return &c2;
      });
      return &cc;
    });
    cond.and_not_in(s(collision_val::kItrKind),
                    {n(itr_num(ItrKind::Block)), n(itr_num(ItrKind::MagicFlute)),
                     n(itr_num(ItrKind::MagicFlute2)), n(itr_num(ItrKind::Pick)),
                     n(itr_num(ItrKind::PickSecretly))});
    cond.and_not_in(s(collision_val::kItrEffect),
                    {n(effect_num(ItrEffect::Ice2)), n(effect_num(ItrEffect::MFire1))});
    if (strict_equals(id, s(oid::kFreezeBall))) {
      cond.and_(s(collision_val::kAttackerIsFreezableBall), u"!=", n(1));
    }
    Object fields;
    fields.set(u"test", Value(cond.done()));
    fields.set(u"actions", make_arr({action_v_next(u"20")}));
    edit_bdy_edit(bdy, Value(std::make_shared<Object>(fields)));
    Value ctx30 = make_bdy_ctx(*c, bdy, -1);
    new_bdy.push_back(cook_ball_bdy_get_hit_to_frame_30(ctx30));
  }
  for (const Value& v : new_bdy.items()) arr->push_back(v);

  const Value itr_list = ensure_field_array(*f, u"itr");
  Array* itrs = as_array_mut(itr_list);
  if (itrs == nullptr) return;
  for (size_t i = 0; i < itrs->size(); ++i) {
    Object* io = as_object(itrs->at(i));
    if (io == nullptr) continue;
    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    CondMaker cond;
    cond.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));
    Value actions = field_or_any(*io, u"actions");
    Object item;
    item.set(u"type", s(action_type::kA_NEXT_FRAME));
    item.set(u"test", Value(cond.done()));
    item.set(u"data", make_obj({{u"id", s(u"10")}}));
    io->set(u"actions", ensure(actions, std::vector<Value>{Value(std::make_shared<Object>(item))}));
  }
}

void cook_ball_frame_state_3005(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value frame = field_or_any(*c, u"frame");
  Object* f = as_object_mut(frame);
  if (f == nullptr) return;

  const Value* bdy_ptr = f->get(u"bdy");
  if (bdy_ptr != nullptr && truthy(*bdy_ptr)) {
    Value bdy_list = *bdy_ptr;
    Array* arr = as_array(bdy_list);
    if (arr != nullptr) {
      for (size_t i = 0; i < arr->size(); ++i) {
        Object* bo = as_object(arr->at(i));
        if (bo == nullptr) continue;
        Value actions = field_or_any(*bo, u"actions");
        if (!truthy(actions)) {
          actions = make_empty_array();
          bo->set(u"actions", actions);
        }
        CondMaker cond;
        cond.add(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::Ball_3005)));
        cond.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
        Object item;
        item.set(u"type", s(action_type::kV_NEXT_FRAME));
        item.set(u"test", Value(cond.done()));
        item.set(u"data", make_obj({{u"id", s(u"20")}}));
        Array* a = as_array(actions);
        if (a != nullptr) a->push_back(Value(std::make_shared<Object>(item)));
      }
    }
  }
  const Value itr_list = field_or_any(*f, u"itr");
  foreach (itr_list, [](Value& item, size_t) {
    Object* io = as_object(item);
    if (io == nullptr) return;
    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) return;
    Value actions = field_or_any(*io, u"actions");
    CondMaker cond;
    cond.add(s(collision_val::kVictimState), u"==", n(state_num(StateEnum::Ball_3005)));
    Object action;
    action.set(u"type", s(action_type::kA_NEXT_FRAME));
    action.set(u"test", Value(cond.done()));
    action.set(u"data", make_obj({{u"id", s(u"20")}}));
    io->set(u"actions", ensure(actions, std::vector<Value>{Value(std::make_shared<Object>(action))}));
  });
}

void cook_ball_frame_state_3006(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return;
  const Value frame = field_or_any(*c, u"frame");
  Object* f = as_object_mut(frame);
  if (f == nullptr) return;
  const Value data = field_or_any(*c, u"data");
  const Object* dobj = as_object_mut(data);
  const Value id = dobj != nullptr ? field_or_any(*dobj, u"id") : Value();
  const Value behavior = field_or_any(*f, u"behavior");
  const bool special = equals(behavior, n(static_cast<double>(FrameBehavior::JohnChase))) ||
                       equals(behavior, n(static_cast<double>(FrameBehavior::JohnBiscuitLeaving))) ||
                       equals(id, s(oid::kJohnBiscuit));

  const Value bdy_list = field_or_any(*f, u"bdy");
  foreach (bdy_list, [&special](Value& item, size_t) {
    Object* bo = as_object(item);
    if (bo == nullptr) return;
    if (special) {
      CondMaker c1;
      c1.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
      CondMaker c2;
      c2.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
      CondMaker c3;
      c3.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
      CondMaker c4;
      c4.one_of(s(collision_val::kAttackerState),
                {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});
      c4.and_(s(collision_val::kItrKind), u"!=", n(itr_num(ItrKind::JohnShield)));
      Array list;
      list.push_back(make_obj({{u"type", s(action_type::kV_REBOUND_VX)},
                               {u"test", Value(c1.done())}}));
      list.push_back(make_obj({{u"type", s(action_type::kV_TURN_FACE)},
                               {u"test", Value(c2.done())}}));
      list.push_back(make_obj({{u"type", s(action_type::kV_TURN_TEAM)},
                               {u"test", Value(c3.done())}}));
      list.push_back(make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                               {u"test", Value(c4.done())},
                               {u"data", make_obj({{u"id", s(u"20")}})}}));
      Value fresh(std::make_shared<Array>(list));
      bo->set(u"actions", fresh);
    } else {
      CondMaker cond;
      cond.one_of(s(collision_val::kAttackerState),
                  {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});
      cond.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
      Value actions = field_or_any(*bo, u"actions");
      Object item;
      item.set(u"type", s(action_type::kV_NEXT_FRAME));
      item.set(u"test", Value(cond.done()));
      item.set(u"data", make_obj({{u"id", s(u"20")}}));
      bo->set(u"actions", ensure(actions, std::vector<Value>{Value(std::make_shared<Object>(item))}));
    }
  });
  const Value itr_list = field_or_any(*f, u"itr");
  foreach (itr_list, [](Value& item, size_t) {
    Object* io = as_object(item);
    if (io == nullptr) return;
    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) return;
    CondMaker cond;
    cond.one_of(s(collision_val::kVictimState),
                {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});
    Value actions = field_or_any(*io, u"actions");
    Object item2;
    item2.set(u"type", s(action_type::kA_NEXT_FRAME));
    item2.set(u"test", Value(cond.done()));
    item2.set(u"data", make_obj({{u"id", s(u"20")}}));
    io->set(u"actions", ensure(actions, std::vector<Value>{Value(std::make_shared<Object>(item2))}));
  });
}

}
}
