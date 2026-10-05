#include "lfw/loader/preprocess_itr.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/loader/preprocess_action.h"
#include "lfw/loader/preprocess_next_frame.h"
#include "lfw/loader/resolve_prefab.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace loader {

using dat_translator::make_obj;
using dat_translator::n;
using dat_translator::s;

namespace {

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// TS 的 `===`：不做数字转换，`"1" !== 1` 因此不匹配任何 case。
bool is_num_eq(const Value& v, ItrKind k) { return strict_equals(v, Value(static_cast<double>(k))); }

bool is_num_eq(const Value& v, ItrEffect e) {
  return strict_equals(v, Value(static_cast<double>(e)));
}

Value num(ItrKind k) { return Value(static_cast<double>(k)); }
Value num(ItrEffect e) { return Value(static_cast<double>(e)); }
Value num(EntityEnum e) { return Value(static_cast<double>(e)); }
Value num(StateEnum st) { return Value(static_cast<double>(st)); }
Value num(WeaponEnum w) { return Value(static_cast<double>(w)); }

}

bool preprocess_itr(Value& ctx, std::u16string& error) {
  Object* const ctx_o = as_object(ctx);
  if (ctx_o == nullptr) return false;

  const Value data = field_or(ctx, u"data");
  Value itr = field_or(ctx, u"itr");

  // TS 的 `resolve_prefab` 一开始就读 `obj.ref`：`null` / `undefined` 时会抛 TypeError。
  if (is_nullish(itr)) return false;

  const ResolvePrefabResult merged = resolve_prefab(itr, field_or(data, u"itr_prefabs"));
  if (!merged.ok) {
    error = prefab_error_message(u"preprocess_itr", to_string(field_or(data, u"id")), u"itr", merged);
    return false;
  }
  itr = merged.value;
  Object* const it = as_mut(itr);
  if (it == nullptr) {
    // 非 `null` 的标量（数字 / 字符串 / 布尔）上读不到任何字段：没有分支会命中、也没有写入，
    // TS 因此不抛。数组同理（用例字面量给数组挂不了键，两边都读不到字段）。
    ctx_o->set(u"itr", itr);
    return true;
  }

  const Value catchingact = field_or(itr, u"catchingact");
  if (truthy(catchingact)) {
    Value local = catchingact;
    if (!preprocess_next_frame(local)) return false;
  }
  const Value caughtact = field_or(itr, u"caughtact");
  if (truthy(caughtact)) {
    Value local = caughtact;
    if (!preprocess_next_frame(local)) return false;
  }

  const auto set_default = [&](const char16_t* key, double v) {
    if (is_nullish(field_or(itr, key))) it->set(std::u16string(key), Value(v));
  };
  const auto test_is_nullish = [&]() { return is_nullish(field_or(itr, u"test")); };
  const auto set_test = [&](const std::u16string& src) { it->set(u"test", Value(src)); };
  const auto set_hit_flag = [&]() {
    const Value hf = field_or(itr, u"hit_flag");
    dat_translator::set_hit_flag(*it,
                                 is_nullish(hf) ? n(static_cast<double>(HitFlag::AllBoth)) : hf);
  };
  const auto vrest_to_arest = [&]() {
    const Value vrest = field_or(itr, u"vrest");
    if (truthy(vrest)) {
      it->set(u"arest", vrest);
      it->remove(u"vrest");
    }
  };

  const Value kind = field_or(itr, u"kind");
  if (is_num_eq(kind, ItrKind::Catch)) {
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    vrest_to_arest();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Tired));
      cm.and_(s(collision_val::kAClosingSpeedX), u">", n(0));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::ForceCatch)) {
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    vrest_to_arest();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Falling));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Normal)) {
    const Value effect = field_or(itr, u"effect");
    if (is_num_eq(effect, ItrEffect::Fire)) {
      if (test_is_nullish()) {
        CondMaker cm;
        cm.add(s(collision_val::kVictimState), u"!=", num(StateEnum::Burning));
        cm.or_(s(collision_val::kAttackerState), u"!=", num(StateEnum::BurnRun));
        set_test(cm.done());
      }
    } else if (is_num_eq(effect, ItrEffect::MFire1)) {
      if (test_is_nullish()) {
        CondMaker cm;
        cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
        cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::BurnRun));
        cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Burning));
        set_test(cm.done());
      }
    } else if (is_num_eq(effect, ItrEffect::MFire2)) {
      if (test_is_nullish()) {
        CondMaker cm;
        cm.add(s(collision_val::kVictimState), u"!=", num(StateEnum::BurnRun));
        cm.and_(s(collision_val::kVictimState), u"!=", num(StateEnum::Burning));
        set_test(cm.done());
      }
    } else if (is_num_eq(effect, ItrEffect::Through)) {
      if (test_is_nullish()) {
        CondMaker cm;
        cm.add(s(collision_val::kVictimType), u"!=", num(EntityEnum::Fighter));
        set_test(cm.done());
      }
    } else if (is_num_eq(effect, ItrEffect::Ice2)) {
      if (test_is_nullish()) {
        CondMaker cm;
        cm.add(s(collision_val::kVictimState), u"!=", num(StateEnum::Frozen));
        cm.and_(s(collision_val::kVictimFrameId), u"!=",
                s(collision_val::kVictimFrameIndex_ICE));
        set_test(cm.done());
      }
    }
  } else if (is_num_eq(kind, ItrKind::Pick)) {
    set_hit_flag();
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kAttackerHasHolding), u"==", n(0));
      cm.and_(s(collision_val::kVictimHasHolder), u"==", n(0));
      cm.and_one_of(s(collision_val::kVictimState),
                    {num(StateEnum::Weapon_OnGround), num(StateEnum::HeavyWeapon_OnGround)});
      set_test(cm.done());
    }
    CondMaker light;
    light.and_(s(collision_val::kVictimBaseType), u"!=", num(WeaponEnum::Heavy));
    CondMaker heavy;
    heavy.and_(s(collision_val::kVictimBaseType), u"==", num(WeaponEnum::Heavy));
    const std::vector<Value> items = {
        make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                  {u"desc", s(u"picking_light 捡起轻型武器")},
                  {u"pretest", Value(true)},
                  {u"test", Value(light.done())},
                  {u"data", make_obj({{u"id", s(u"115")}})}}),
        make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                  {u"pretest", Value(true)},
                  {u"test", Value(heavy.done())},
                  {u"desc", s(u"picking_heavy 捡起重型武器")},
                  {u"data", make_obj({{u"id", s(u"117")}})}})};
    Value acts = field_or(itr, u"actions");
    // TS `ensure(actions, ...)`：`actions` 不是假值又不是数组时 `output.push` 不是函数 ⇒ 抛。
    if (truthy(acts) && as_array(acts) == nullptr) return false;
    it->set(u"actions", ensure(acts, items));
  } else if (is_num_eq(kind, ItrKind::PickSecretly)) {
    set_hit_flag();
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kAttackerHasHolder), u"==", n(0));
      cm.and_(s(collision_val::kVictimHasHolder), u"==", n(0));
      cm.and_(s(collision_val::kVictimState), u"==", num(StateEnum::Weapon_OnGround));
      cm.and_(s(collision_val::kAHitAttack), u"==", n(1));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::SuperPunchMe)) {
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::MagicFlute) || is_num_eq(kind, ItrKind::MagicFlute2)) {
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolfWeapon));
        return &c;
      });
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Block)) {
    set_hit_flag();
    set_default(u"motionless", 0);
    set_default(u"shaking", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kBdyKind), u"==", n(static_cast<double>(BdyKind::Normal)));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::JohnShield)) {
    set_hit_flag();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.and_(s(collision_val::kVictimType), u"!=", num(EntityEnum::Fighter));
      cm.or_(s(collision_val::kSameTeam), u"!=", n(1));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Heal)) {
    set_hit_flag();
    const Value dvx = field_or(itr, u"dvx");
    if (truthy(dvx)) {
      Value acts = field_or(itr, u"actions");
      if (truthy(acts) && as_array(acts) == nullptr) return false;
      const std::vector<Value> items = {
          make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                    {u"data", dat_translator::get_next_frame_by_raw_id(dvx, u"frame")}})};
      it->set(u"actions", ensure(acts, items));
    }
    if (test_is_nullish()) {
      CondMaker cm;
      cm.and_(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Freeze)) {
    set_hit_flag();
    set_default(u"shaking", 0);
    set_default(u"motionless", 0);
    set_default(u"dvx", 0);
    set_default(u"dvy", 0);
    set_default(u"dvz", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
      cm.and_([&](CondMaker& c) {
        c.add(s(collision_val::kSameTeam), u"==", n(0));
        c.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));
        return &c;
      });
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::Whirlwind)) {
    set_hit_flag();
    set_default(u"shaking", 0);
    set_default(u"motionless", 0);
    set_default(u"vrest", 1);
    if (is_nullish(field_or(itr, u"injury"))) it->set(u"injury", Value());
    set_default(u"dvx", 0);
    set_default(u"dvy", 0);
    set_default(u"dvz", 0);
    if (test_is_nullish()) {
      CondMaker cm;
      cm.wrap([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Weapon));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kHenryArrow1));
        c.and_(s(collision_val::kVictimOID), u"!=", s(oid::kRudolf));
        return &c;
      });
      cm.or_([&](CondMaker& c) {
        c.add(s(collision_val::kVictimType), u"==", num(EntityEnum::Fighter));
        c.and_([&](CondMaker& c2) {
          c2.add(s(collision_val::kSameTeam), u"==", n(0));
          c2.or_(s(collision_val::kVictimState), u"==", num(StateEnum::Frozen));
          return &c2;
        });
        return &c;
      });
      set_test(cm.done());
    }
  } else if (is_num_eq(kind, ItrKind::CharacterThrew)) {
    set_hit_flag();
    if (test_is_nullish()) {
      CondMaker cm;
      cm.add(s(collision_val::kAttackerThrew), u"==", n(1));
      cm.and_(s(collision_val::kAttackerType), u"==", num(EntityEnum::Fighter));
      set_test(cm.done());
    }
  }

  Value actions = field_or(itr, u"actions");
  if (truthy(actions)) {
    Array* const a = as_array(actions);
    if (a == nullptr) return false;
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_action(item)) return false;
    }
  }

  const Value test = field_or(itr, u"test");
  if (truthy(test)) it->set(u"__tester", test);

  ctx_o->set(u"itr", itr);
  return true;
}

}
}
