#include "lfw/dat_translator/make_weapon_special.h"

#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/broken_piece_frames.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/opoint_kind.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/traversal.h"
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

std::set<const Object*>& handled() {
  static std::set<const Object*> s;
  return s;
}

std::vector<Value> repeat_of(double count, const Value& item) {
  std::vector<Value> out;
  for (double i = 0; i < count; i += 1) out.push_back(item);
  return out;
}

Value make_aa(size_t idx) {
  switch (idx % 10) {
    case 0:
      return make_obj({{u"dvy", n(5)}, {u"dvx", n(-1)}});
    case 1:
      return make_obj({{u"dvy", n(5)}, {u"dvx", n(1)}});
    case 2:
      return make_obj({{u"dvy", n(3)}});
    case 3:
      return make_obj({{u"dvy", n(2)}, {u"dvx", n(2)}});
    case 4:
      return make_obj({{u"dvy", n(2)}, {u"dvx", n(-2)}});
    case 5:
      return make_obj({{u"dvy", n(4)}, {u"dvx", n(-1.5)}});
    case 6:
      return make_obj({{u"dvy", n(4)}, {u"dvx", n(1.5)}});
    case 7:
      return make_obj({{u"dvy", n(2)}});
    case 8:
      return make_obj({{u"dvy", n(1)}, {u"dvx", n(1)}});
    default:
      return make_obj({{u"dvy", n(1)}, {u"dvx", n(-1)}});
  }
}

Value broken_pieces_opoints(const std::vector<Value>& frame_ids) {
  Array out;
  for (size_t idx = 0; idx < frame_ids.size(); ++idx) {
    Object o;
    o.set(u"kind", en(OpointKind::Normal));
    o.set(u"x", n(0));
    o.set(u"y", n(-1));
    o.set(u"pos_type", n(1));
    o.set(u"action", make_obj({{u"id", frame_ids[idx]}}));
    o.set(u"oid", s(u"999"));
    o.set(u"unimportant", n(1));
    o.set(u"inherit_speed_x", n(0.5));
    o.set(u"inherit_speed_y", n(0.5));
    o.set(u"inherit_speed_z", n(0.5));
    const Value aa_v = make_aa(idx);
    const Object* aa = as_object(aa_v);
    if (aa != nullptr) {
      for (const std::u16string& k : aa->keys()) {
        const Value* v = aa->get(k);
        if (v != nullptr) o.set(k, *v);
      }
    }
    out.push_back(Value(std::make_shared<Object>(o)));
  }
  return Value(std::make_shared<Array>(out));
}

void set_or_keep_weight(Object& base, double fallback) {
  if (is_nullish(field_or_any(base, u"weight"))) base.set(u"weight", n(fallback));
}

void set_or_keep_brokens(Object& base, const std::vector<Value>& ids) {
  if (is_nullish(field_or_any(base, u"brokens"))) {
    base.set(u"brokens", broken_pieces_opoints(ids));
  }
}

Value opoint_frame(const char16_t* name) {
  const Value* v = defines::find(std::u16string(name));
  return v != nullptr ? *v : Value();
}

}

void make_weapon_special(Value& data) {
  Object* d = as_object(data);
  if (d == nullptr) return;
  const Value* id_v = d->get(u"id");
  const double num_data_id = to_number(field_or_any(*d, u"id"));
  const Value* base_v = d->get(u"base");
  Object* base = base_v != nullptr ? const_cast<Object*>(as_object(*base_v)) : nullptr;

  if (base != nullptr && num_data_id >= 100 && num_data_id <= 199) {
    Value cur = field_or_any(*base, u"group");
    base->set(u"group", ensure(cur, std::vector<Value>{s(entity_group::kVsWeapon),
                                                       s(entity_group::kStageWeapon)}));
  }

  if (base != nullptr) {
    const Value type_v = field_or_any(*base, u"type");
    if (strict_equals(type_v, en(WeaponEnum::Heavy))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(200));
    } else if (strict_equals(type_v, en(WeaponEnum::Knife))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(70));
    } else if (strict_equals(type_v, en(WeaponEnum::Stick))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(100));
    } else if (strict_equals(type_v, en(WeaponEnum::Baseball)) ||
               strict_equals(type_v, en(WeaponEnum::Drink))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(100));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(200));
    }
  }

  if (id_v == nullptr || !is_str(*id_v)) return;
  const std::u16string id = std::get<std::u16string>(*id_v);
  Value* frames_v = const_cast<Value*>(d->get(u"frames"));

  if (id == oid::kHenryArrow1) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_ARROW")));
      base->remove(u"group");
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& value) {
        Object* frame = as_object(value);
        if (frame == nullptr) return;
        const Value* state_v = frame->get(u"state");
        if (state_v != nullptr && strict_equals(*state_v, en(StateEnum::Weapon_Rebounding))) {
          frame->remove(u"itr");
          return;
        }
        Value* itr_v = const_cast<Value*>(frame->get(u"itr"));
        const Array* arr = itr_v != nullptr ? as_array(*itr_v) : nullptr;
        if (arr == nullptr) return;
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* itr = const_cast<Object*>(as_object(arr->at(i)));
          if (itr == nullptr) continue;
          if (handled().count(itr) != 0) continue;
          handled().insert(itr);
          CondMaker cond;
          cond.add(s(collision_val::kVictimType), u"==", en(EntityEnum::Fighter));
          cond.and_(s(collision_val::kBdyKind), u"==", en(BdyKind::Normal));
          cond.and_(s(collision_val::kVictimState), u"!=", en(StateEnum::Defend));
          cond.and_([&](CondMaker& cc) {
            cc.add(s(collision_val::kArmorWork), u"==", n(0));
            cc.or_(s(collision_val::kVToughness), u"<=", n(0));
            return &cc;
          });
          Value actions = field_or_any(*itr, u"actions");
          const Value item = make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                                       {u"data", opoint_frame(u"Defines.NEXT_FRAME_GONE")},
                                       {u"pretest", Value(true)},
                                       {u"test", Value(cond.done())}});
          itr->set(u"actions", ensure(actions, item));
        }
      });
    }
    return;
  }

  if (id == oid::kRudolfWeapon) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_ARROW")));
      base->remove(u"group");
    }
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& value) {
        Object* frame = as_object(value);
        if (frame == nullptr) return;
        const Value* state_v = frame->get(u"state");
        if (state_v != nullptr && strict_equals(*state_v, en(StateEnum::Weapon_Rebounding))) {
          frame->remove(u"itr");
        }
      });
    }
    return;
  }

  if (base == nullptr) return;

  if (id == oid::kWeapon_Stick) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_NOMRAL"));
    set_or_keep_brokens(*base, {stick(), stick(), sstick(), sstick(), sstick()});
    return;
  }

  if (id == oid::kWeapon_Hoe) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_HOE"));
    set_or_keep_brokens(*base, {k_hoe(), k_hoe(), hoe(), hoe(), s_hoe(), s_hoe()});
    return;
  }

  if (id == oid::kWeapon_Knife) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_LIGHT"));
    set_or_keep_brokens(*base, {k_hoe(), k_hoe(), s_hoe(), s_hoe()});
    return;
  }

  if (id == oid::kWeapon_baseball) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_BASEBALL"));
    set_or_keep_brokens(*base, repeat_of(5, baseball()));
    return;
  }

  if (id == oid::kWeapon_milk) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_LIGHT"));
    std::vector<Value> ids;
    ids.push_back(milk1());
    for (const Value& v : repeat_of(4, milk3())) ids.push_back(v);
    for (const Value& v : repeat_of(5, milk2())) ids.push_back(v);
    set_or_keep_brokens(*base, ids);
    base->set(u"group", make_arr({s(entity_group::kVsWeapon)}));
    base->set(u"drink", make_obj({{u"hp_h_total", n(160)},
                                  {u"hp_h_value", n(4)},
                                  {u"hp_h_ticks", n(5)},
                                  {u"hp_r_total", n(80)},
                                  {u"hp_r_value", n(2)},
                                  {u"hp_r_ticks", n(5)},
                                  {u"mp_h_total", n(166)},
                                  {u"mp_h_value", n(5)},
                                  {u"mp_h_ticks", n(6)}}));
    return;
  }

  if (id == oid::kWeapon_Stone) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_HEAVY"));
    set_or_keep_brokens(*base, {stone(), stone(), sstone(), sstone(), sstone()});
    return;
  }

  if (id == oid::kWeapon_WoodenBox) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_HEAVY"));
    set_or_keep_brokens(*base, {box0(), box1(), box2(), box3(), box3()});
    return;
  }

  if (id == oid::kWeapon_Beer) {
    base->set(u"group", make_arr({s(entity_group::kVsWeapon)}));
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_LIGHT"));
    set_or_keep_brokens(*base, {beer1(), beer2(), beer2(), beer2(), beer2(), milk2(), milk2(),
                                milk2(), milk2(), milk2()});
    {
      Value cur = field_or_any(*base, u"group");
      base->set(u"group", ensure(cur, s(entity_group::kVsWeapon)));
    }
    base->set(u"drink", make_obj({{u"mp_h_total", n(750)},
                                  {u"mp_h_value", n(12)},
                                  {u"mp_h_ticks", n(1)}}));
    return;
  }

  if (id == oid::kWeapon_Boomerang) {
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_LIGHT"));
    set_or_keep_brokens(*base, {boomerang(), boomerang(), boomerang()});
    return;
  }

  if (id == oid::kWeapon_LouisArmourA) {
    base->remove(u"group");
    base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_HEAVY")));
    base->set(u"brokens",
              broken_pieces_opoints({armour(), armour(), armour(), armour(), armour()}));
    return;
  }

  if (id == oid::kWeapon_LouisArmourB) {
    base->remove(u"group");
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_HEAVY"));
    set_or_keep_brokens(*base, {armour(), armour(), armour(), armour(), armour()});
    return;
  }

  if (id == oid::kWeapon_IceSword) {
    base->remove(u"group");
    {
      Value cur = field_or_any(*base, u"group");
      base->set(u"group", ensure(cur, s(entity_group::kFreezer)));
    }
    set_or_keep_weight(*base, defines::num(u"Defines.WEAPON_WEIGHT_NOMRAL"));
    set_or_keep_brokens(*base, {icesword1(), icesword1(), icesword1(), icesword2(), icesword2(),
                                icesword2(), icesword2()});
    return;
  }
}

}
}
