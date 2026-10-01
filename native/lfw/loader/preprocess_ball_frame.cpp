#include "lfw/loader/preprocess_ball_frame.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/dat_translator/ball_frame_state.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/foreach.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace loader {

using dat_translator::make_arr;
using dat_translator::make_obj;
using dat_translator::s;

namespace {

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

Value field_at(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  if (o == nullptr) return Value();
  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();
}

bool kind_is(const Value& itr, ItrKind k) {
  return strict_equals(field_at(itr, u"kind"), Value(static_cast<double>(k)));
}

bool state_is(const Value& frame, StateEnum k) {
  return strict_equals(field_at(frame, u"state"), Value(static_cast<double>(k)));
}

}

void preprocess_ball_frame(Value& ctx) {
  Object* ctx_o = as_object(ctx);
  if (ctx_o == nullptr) return;
  Value frame = field_at(ctx, u"frame");
  Object* frame_o = as_mut(frame);
  const Value data = field_at(ctx, u"data");
  const Value base = field_at(data, u"base");
  if (frame_o == nullptr) return;

  const Value itr_v = field_at(frame, u"itr");
  if (Array* itrs = const_cast<Array*>(as_array(itr_v))) {
    const size_t n = itrs->size();
    for (size_t i = 0; i < n; ++i) {
      Value itr = itrs->at(i);
      Object* io = as_mut(itr);
      if (io == nullptr) continue;
      if (kind_is(itr, ItrKind::JohnShield)) {
        const Value on_dead = field_at(frame, u"on_dead");
        if (truthy(on_dead)) {
          CondMaker cm;
          cm.add(Value(std::u16string(collision_val::kVictimType)), u"==",
                 Value(static_cast<double>(EntityEnum::Fighter)));
          const std::vector<Value> items = {make_obj({{u"type", s(u"A_NEXT_FRAME")},
                                                      {u"test", Value(cm.done())},
                                                      {u"data", on_dead}})};
          Value acts = field_at(itr, u"actions");
          io->set(u"actions", ensure(acts, items));
        }
      }
      const Value hit_sounds = field_at(base, u"hit_sounds");
      const Array* hs = as_array(hit_sounds);
      const Value hit_sound = hs != nullptr && hs->size() > 0 ? hs->at(0) : Value();
      if (truthy(hit_sound) && !kind_is(itr, ItrKind::Whirlwind) &&
          !kind_is(itr, ItrKind::Freeze) && !kind_is(itr, ItrKind::Block) &&
          !kind_is(itr, ItrKind::Heal)) {
        const std::vector<Value> items = {
            make_obj({{u"type", s(u"A_SOUND")},
                      {u"data", make_obj({{u"path", make_arr({hit_sound})}})}})};
        Value acts = field_at(itr, u"actions");
        io->set(u"actions", ensure(acts, items));
      }
    }
  }

  const Value grav = field_at(frame, u"gravity_enabled");
  if (std::holds_alternative<std::monostate>(grav) || std::holds_alternative<NullTag>(grav)) {
    frame_o->set(u"gravity_enabled", Value(false));
  }

  if (state_is(frame, StateEnum::Ball_Flying)) {
    dat_translator::cook_ball_frame_state_3000(ctx);
  } else if (state_is(frame, StateEnum::Ball_Hitting)) {
    dat_translator::cook_ball_frame_state_3001(ctx);
  } else if (state_is(frame, StateEnum::Ball_3005)) {
    dat_translator::cook_ball_frame_state_3005(ctx);
  } else if (state_is(frame, StateEnum::Ball_3006)) {
    dat_translator::cook_ball_frame_state_3006(ctx);
  } else {
    dat_translator::cook_ball_frame_state_15(ctx);
  }

  const Value itr_v2 = field_at(frame, u"itr");
  if (Array* itrs = const_cast<Array*>(as_array(itr_v2))) {
    const size_t n = itrs->size();
    for (size_t i = 0; i < n; ++i) {
      Value itr = itrs->at(i);
      Object* io = as_mut(itr);
      if (io == nullptr) continue;
      if (kind_is(itr, ItrKind::Normal) || kind_is(itr, ItrKind::JohnShield) ||
          kind_is(itr, ItrKind::CharacterThrew) || kind_is(itr, ItrKind::WeaponSwing)) {
        const Value hit_sounds = field_at(base, u"hit_sounds");
        const Array* hs = as_array(hit_sounds);
        if (hs != nullptr && hs->size() > 0) {
          const std::vector<Value> items = {
              make_obj({{u"type", s(u"A_SOUND")},
                        {u"data", make_obj({{u"path", hit_sounds}})}})};
          Value acts = field_at(itr, u"actions");
          io->set(u"actions", ensure(acts, items));
        }
      }
    }
  }
}

}
}
