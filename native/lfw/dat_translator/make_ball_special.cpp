#include "lfw/dat_translator/make_ball_special.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/opoint_kind.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/find.h"
#include "lfw/utils/container_help/traversal.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

Value field_or_any(const Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? *v : Value();
}

Object* frame_at(const Object* frames, const char16_t* key) {
  const Value* v = frames != nullptr ? frames->get(std::u16string(key)) : nullptr;
  return v != nullptr ? const_cast<Object*>(as_object(*v)) : nullptr;
}

Array* array_of(Object& o, const char16_t* key) {
  const Value* v = o.get(std::u16string(key));
  return v != nullptr ? const_cast<Array*>(as_array(*v)) : nullptr;
}

double pic_half(const Object& f, const char16_t* axis) {
  const Value* pic_v = f.get(u"pic");
  const Object* pic = pic_v != nullptr ? as_object(*pic_v) : nullptr;
  if (pic == nullptr) return 0.0;
  const Value* v = pic->get(std::u16string(axis));
  return floor(v != nullptr ? to_number(*v) / 2 : 0.0);
}

size_t tail_index(const Object& f, const char16_t* tail_oid) {
  const Value* op_v = f.get(u"opoint");
  const Array* arr = op_v != nullptr ? as_array(*op_v) : nullptr;
  return find_value_index(arr, [tail_oid](const Value& v) {
    const Object* o = as_object(v);
    if (o == nullptr) return false;
    const Value* oid_v = o->get(u"oid");
    if (oid_v == nullptr || !strict_equals(*oid_v, s(tail_oid))) return false;
    const Value* act_v = o->get(u"action");
    const Object* act = act_v != nullptr ? as_object(*act_v) : nullptr;
    const Value* id_v = act != nullptr ? act->get(u"id") : nullptr;
    return id_v != nullptr && strict_equals(*id_v, s(u"40"));
  });
}

void edit_tail(Object& f, const char16_t* tail_oid) {
  Value* op_v = const_cast<Value*>(f.get(u"opoint"));
  Array* arr = op_v != nullptr ? const_cast<Array*>(as_array(*op_v)) : nullptr;
  const size_t idx = tail_index(f, tail_oid);
  if (idx == SIZE_MAX || arr == nullptr) return;
  Object* t = as_object(arr->at(idx));
  if (t == nullptr) return;
  t->set(u"unimportant", n(1));
  t->set(u"dvz", n(0));
  t->set(u"dvy", n(0));
  t->set(u"dvx", n(0));
  t->set(u"speedz", n(0));
  t->set(u"ghost", n(1));
}

}

void make_ball_special(Value& data) {
  Object* d = as_object(data);
  if (d == nullptr) return;
  const Value* id_v = d->get(u"id");
  if (id_v == nullptr || !is_str(*id_v)) return;
  const std::u16string id = std::get<std::u16string>(*id_v);
  Value* frames_v = const_cast<Value*>(d->get(u"frames"));
  Object* frames = frames_v != nullptr ? as_object(*frames_v) : nullptr;
  const Value* base_v = d->get(u"base");
  Object* base = base_v != nullptr ? const_cast<Object*>(as_object(*base_v)) : nullptr;

  if (id == oid::kFirenFlame) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        Array* arr = array_of(*fo, u"itr");
        if (arr == nullptr) return;
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* io = as_object(arr->at(i));
          if (io != nullptr) set_hit_flag(*io, en(HitFlag::AllEnemy));
        }
      });
    }
    return;
  }

  if (id == oid::kFirzenBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow", n(1));
        fo->set(u"dvz", n(0));
        fo->set(u"vzm", en(SpeedMode::Fixed));
      });
    }
    return;
  }

  if (id == oid::kBatBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow", n(1));
        fo->set(u"dvz", n(0));
        fo->set(u"vzm", en(SpeedMode::Fixed));
      });
    }
    return;
  }

  if (id == oid::kJanChase) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }
    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr != nullptr) fr->set(u"invisible", field_or_any(*fr, u"wait"));
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* f = as_object(frame);
        if (f == nullptr) return;
        const Value* beh = f->get(u"behavior");
        if (beh != nullptr && strict_equals(*beh, en(FrameBehavior::ChasingSameEnemy))) {
          Value cur = field_or_any(*f, u"opoint");
          const Value item = make_obj({{u"oid", s(oid::kJanChase)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", field_or_any(*f, u"centerx")},
                                       {u"y", field_or_any(*f, u"centery")},
                                       {u"kind", en(OpointKind::Normal)},
                                       {u"ghost", n(1)},
                                       {u"speedz", n(0)},
                                       {u"dvx", n(0)},
                                       {u"dvy", n(0)},
                                       {u"dvz", n(0)},
                                       {u"interval", n(1)},
                                       {u"interval_id", s(u"1")},
                                       {u"interval_mode", n(1)},
                                       {u"unimportant", n(1)}});
          f->set(u"opoint", ensure(cur, item));
        }
        edit_tail(*f, oid::kJanChase);
      });
    }
    return;
  }

  if (id == oid::kJanChaseh) {
    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || is_nullish(*inv_v)) {
        fr->set(u"invisible", field_or_any(*fr, u"wait"));
      }
      const Value* invl_v = fr->get(u"invulnerable");
      if (invl_v == nullptr || is_nullish(*invl_v)) {
        fr->set(u"invulnerable", field_or_any(*fr, u"invisible"));
      }
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* f = as_object(frame);
        if (f == nullptr) return;
        edit_tail(*f, oid::kJanChaseh);
      });
    }
    return;
  }

  if (id == oid::kFirzenChasef) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }
    for (const char16_t* key : {u"59", u"80", u"81"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || is_nullish(*inv_v)) {
        fr->set(u"invisible", field_or_any(*fr, u"wait"));
      }
      const Value* invl_v = fr->get(u"invulnerable");
      if (invl_v == nullptr || is_nullish(*invl_v)) {
        fr->set(u"invulnerable", field_or_any(*fr, u"invisible"));
      }
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* f = as_object(frame);
        if (f == nullptr) return;
        const Value* beh = f->get(u"behavior");
        if (beh != nullptr && strict_equals(*beh, en(FrameBehavior::ChasingSameEnemy))) {
          Value cur = field_or_any(*f, u"opoint");
          const Value item = make_obj({{u"oid", s(oid::kFirzenChasef)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", n(pic_half(*f, u"w"))},
                                       {u"y", n(pic_half(*f, u"h"))},
                                       {u"kind", en(OpointKind::Normal)},
                                       {u"ghost", n(1)},
                                       {u"speedz", n(0)},
                                       {u"dvx", n(0)},
                                       {u"dvy", n(0)},
                                       {u"dvz", n(0)},
                                       {u"interval", n(1)},
                                       {u"interval_id", s(u"1")},
                                       {u"interval_mode", n(1)},
                                       {u"unimportant", n(1)}});
          f->set(u"opoint", ensure(cur, item));
        }
        edit_tail(*f, oid::kFirzenChasef);
      });
    }
    return;
  }

  if (id == oid::kFirzenChasei) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* f = as_object(frame);
        if (f == nullptr) return;
        const Value* beh = f->get(u"behavior");
        if (beh != nullptr && strict_equals(*beh, en(FrameBehavior::ChasingSameEnemy))) {
          Value cur = field_or_any(*f, u"opoint");
          const Value item = make_obj({{u"oid", s(oid::kFirzenChasei)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", n(pic_half(*f, u"w"))},
                                       {u"y", n(pic_half(*f, u"h"))},
                                       {u"kind", en(OpointKind::Normal)},
                                       {u"ghost", n(1)},
                                       {u"speedz", n(0)},
                                       {u"dvx", n(0)},
                                       {u"dvy", n(0)},
                                       {u"dvz", n(0)},
                                       {u"interval", n(1)},
                                       {u"interval_id", s(u"1")},
                                       {u"interval_mode", n(1)},
                                       {u"unimportant", n(1)}});
          f->set(u"opoint", ensure(cur, item));
        }
      });
    }
    return;
  }

  if (id == oid::kDeepBall || id == oid::kDennisBall || id == oid::kWoodyBall ||
      id == oid::kDavisBall || id == oid::kDennisChase || id == oid::kJackBall ||
      id == oid::kJohnBall) {
    if (base != nullptr) {
      Value cur = field_or_any(*base, u"group");
      base->set(u"group", ensure(cur, s(entity_group::kFreezableBall)));
    }
    return;
  }

  if (id == oid::kJohnBiscuit) return;

  if (id == oid::kFreezeBall) {
    if (base != nullptr) {
      Value cur = field_or_any(*base, u"group");
      base->set(u"group", ensure(cur, s(entity_group::kFreezer)));
    }
    return;
  }

  if (id == oid::kBatChase) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        const Value* beh = fo->get(u"behavior");
        if (beh == nullptr || !strict_equals(*beh, en(FrameBehavior::Bat))) return;
        Array* arr = array_of(*fo, u"itr");
        if (arr == nullptr) return;
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* io = as_object(arr->at(i));
          if (io == nullptr) continue;
          Value cur = field_or_any(*io, u"actions");
          const Value item = make_obj(
              {{u"type", s(action_type::kVALUE_STEAL)},
               {u"data", make_obj({{u"target", n(1)},
                                   {u"itr_hp_ratio", n(0.2)},
                                   {u"itr_hp_r_ratio", n(0.2)}})}});
          io->set(u"actions", ensure(cur, item));
        }
      });
    }
    return;
  }
}

}
}
