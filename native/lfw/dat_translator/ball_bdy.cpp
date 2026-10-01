#include "lfw/dat_translator/ball_bdy.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/json.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/labels.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/fields.h"

namespace lfw {
namespace dat_translator {
namespace {

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

Value deep_copy(const Value& v) {
  const std::optional<std::u16string> text = json_stringify(v);
  if (!text.has_value()) return Value();
  const std::optional<Value> parsed = json_parse(*text);
  return parsed.has_value() ? *parsed : Value();
}

Value& edit_bdy_deco(Value& raw) {
  Object* o = as_object(raw);
  if (o == nullptr) return raw;
  const Value* hf = o->get(u"hit_flag");
  if (hf != nullptr && !is_undefined(*hf)) {
    o->set(u"hit_flag_name", Value(defines::get_hit_flag_name(*hf)));
  } else {
    o->remove(u"hit_flag");
    o->remove(u"hit_flag_name");
  }
  const Value* kind = o->get(u"kind");
  if (kind != nullptr && !is_undefined(*kind)) {
    o->set(u"kind_name", Value(defines::bdy_kind_name(*kind)));
  } else {
    o->remove(u"kind");
    o->remove(u"kind_name");
  }
  reorder_fields(raw, bdy_info_fields());
  return raw;
}

void assign_fields(Object& raw, const Value& fields) {
  const Object* fo = as_object(fields);
  if (fo == nullptr) return;
  const std::vector<std::u16string> keys = fo->keys();
  for (const std::u16string& k : keys) {
    const Value* v = fo->get(k);
    raw.set(k, v != nullptr ? *v : Value());
  }
}

double state_num(StateEnum s) { return static_cast<double>(s); }

double itr_num(ItrKind k) { return static_cast<double>(k); }

double ent_num(EntityEnum e) { return static_cast<double>(e); }

}

Value edit_bdy_edit(Value& bdy, const Value& fields) {
  Object* o = as_object(bdy);
  if (o == nullptr) return bdy;
  assign_fields(*o, fields);
  return edit_bdy_deco(bdy);
}

Value edit_bdy_clone(Value& bdy, const Value& fields) {
  Value copy = deep_copy(bdy);
  Object* o = as_object(copy);
  if (o != nullptr) assign_fields(*o, fields);
  edit_bdy_deco(copy);
  return copy;
}

Value cook_ball_bdy_get_hit_to_frame_20(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return Value();
  const Value* bdy_ptr = c->get(u"bdy");
  Value bdy = bdy_ptr != nullptr ? *bdy_ptr : Value();
  const Value* data_ptr = c->get(u"data");
  const Value data = data_ptr != nullptr ? *data_ptr : Value();
  const Object* dobj = as_object(data);
  const Value* id_ptr = dobj != nullptr ? dobj->get(u"id") : nullptr;
  const Value id = id_ptr != nullptr ? *id_ptr : Value();

  CondMaker co;
  if (equals(id, s(oid::kFreezeColumn))) {
    co.add(s(collision_val::kAEmitter), u"!=", s(collision_val::kVEmitter));
    co.and_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::Normal)));
  } else {
    co.add(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::Normal)));
    co.and_([&](CondMaker& cc) {
      cc.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));
      cc.or_(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Entity)));
      cc.or_([&](CondMaker& c2) {
        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Weapon)));
        c2.and_(s(collision_val::kAttackerState), u"!=", n(state_num(StateEnum::Weapon_OnHand)));
        return &c2;
      });
      cc.or_([&](CondMaker& c2) {
        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Fighter)));
        c2.and_(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::BurnRun)));
        return &c2;
      });
      return &cc;
    });
  }
  if (strict_equals(id, s(oid::kFreezeBall))) {
    co.and_(s(collision_val::kAttackerIsFreezableBall), u"!=", n(1));
  }
  Object fields;
  fields.set(u"test", Value(co.done()));
  fields.set(u"actions", make_arr({make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                                             {u"data", make_obj({{u"id", s(u"20")}})}})}) );
  return edit_bdy_clone(bdy, Value(std::make_shared<Object>(fields)));
}

Value cook_ball_bdy_get_hit_to_frame_30(Value& ctx) {
  Object* c = as_object(ctx);
  if (c == nullptr) return Value();
  const Value* bdy_ptr = c->get(u"bdy");
  Value bdy = bdy_ptr != nullptr ? *bdy_ptr : Value();

  CondMaker co;
  co.add(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));
  co.or_([&](CondMaker& cc) {
    cc.one_of(s(collision_val::kItrKind),
              {n(itr_num(ItrKind::Normal)), n(itr_num(ItrKind::CharacterThrew)),
               n(itr_num(ItrKind::WeaponSwing))});
    cc.and_([&](CondMaker& c2) {
      c2.add([&](CondMaker& c3) {
        c3.add(s(collision_val::kSameTeam), u"==", n(0));
        c3.and_([&](CondMaker& c4) {
          c4.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Fighter)));
          c4.or_([&](CondMaker& c5) {
            c5.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Weapon)));
            c5.and_(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::Weapon_OnHand)));
            return &c5;
          });
          return &c4;
        });
        return &c3;
      });
      c2.or_([&](CondMaker& c3) {
        c3.add(s(collision_val::kSameTeam), u"==", n(1));
        c3.and_(s(collision_val::kSameFacing), u"==", n(0));
        c3.and_([&](CondMaker& c4) {
          c4.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Fighter)));
          c4.or_([&](CondMaker& c5) {
            c5.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Weapon)));
            c5.and_(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::Weapon_OnHand)));
            return &c5;
          });
          return &c4;
        });
        return &c3;
      });
      return &c2;
    });
    return &cc;
  });
  Object fields;
  fields.set(u"hit_flag", n(static_cast<double>(HitFlag::AllBoth)));
  fields.set(u"test", Value(co.done()));
  fields.set(u"actions", make_arr({make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                                             {u"data", make_obj({{u"id", s(u"30")}})}})}) );
  return edit_bdy_clone(bdy, Value(std::make_shared<Object>(fields)));
}

}
}
