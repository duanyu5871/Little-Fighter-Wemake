#include "lfw/loader/preprocess_bdy.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/loader/preprocess_action.h"
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

// `defines/BdyKind.ts` 的 OLD_BDY_KIND_GOTO_MIN/MAX：`kind` 落在 [1000, 1999] 时按“老式跳转”
// 处理（被击中后跳到 `kind - 1000` 帧）。这两个常量不在生成的枚举表里。
constexpr double kGotoMin = 1000;
constexpr double kGotoMax = 1999;

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// TS 的 `===`：不做数字转换，`"0" !== 0`。
bool is_num_eq(const Value& v, double k) { return strict_equals(v, Value(k)); }

// TS 的 `frame?.state`：`frame` 不是对象时读不到 `state`。
Value state_of(const Value& frame) { return field_or(frame, u"state"); }

}

bool preprocess_bdy(Value& ctx, std::u16string& error) {
  Object* const ctx_o = as_object(ctx);
  if (ctx_o == nullptr) return false;

  const Value data = field_or(ctx, u"data");
  const Value frame = field_or(ctx, u"frame");
  Value bdy = field_or(ctx, u"bdy");

  const ResolvePrefabResult merged = resolve_prefab(bdy, field_or(data, u"bdy_prefabs"));
  if (!merged.ok) {
    error = prefab_error_message(u"preprocess_bdy", to_string(field_or(data, u"id")), u"bdy", merged);
    return false;
  }
  bdy = merged.value;
  Object* const b = as_mut(bdy);
  if (b == nullptr) return false;

  const Value kind = field_or(bdy, u"kind");
  if (is_num_eq(kind, static_cast<double>(BdyKind::Normal)) &&
      is_num_eq(state_of(frame), static_cast<double>(StateEnum::Caught)) &&
      is_nullish(field_or(bdy, u"hit_flag"))) {
    dat_translator::set_hit_flag(*b, n(static_cast<double>(HitFlag::AllBoth)));
  }

  if (ge(kind, n(kGotoMin)) && le(kind, n(kGotoMax))) {
    dat_translator::set_bdy_kind(*b, n(static_cast<double>(BdyKind::Criminal)));

    CondMaker cm;
    cm.add([&](CondMaker& c) {
      c.add(s(collision_val::kSameTeam), u"==", n(0));
      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Fighter)));
      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Normal)));
      return &c;
    });
    cm.or_([&](CondMaker& c) {
      c.add(s(collision_val::kSameTeam), u"==", n(0));
      c.and_(s(collision_val::kAttackerType), u"==", n(static_cast<double>(EntityEnum::Weapon)));
      c.and_(s(collision_val::kItrKind), u"==", n(static_cast<double>(ItrKind::Normal)));
      c.and_([&](CondMaker& c2) {
        c2.or_(s(collision_val::kAttackerState), u"==",
               n(static_cast<double>(StateEnum::Weapon_OnHand)));
        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kHenryArrow1));
        c2.or_(s(collision_val::kAttackerOID), u"==", s(oid::kRudolfWeapon));
        return &c2;
      });
      return &c;
    });
    b->set(u"test", Value(cm.done()));

    Value acts = field_or(bdy, u"actions");
    // TS `ensure(actions, ...)`：`actions` 不是假值又不是数组时 `output.push` 不是函数 ⇒ 抛。
    if (truthy(acts) && as_array(acts) == nullptr) return false;
    const std::vector<Value> items = {
        make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                  {u"data", make_obj({{u"id", Value(to_string(Value(to_number(kind) - 1000)))}})}}),
        make_obj({{u"type", s(action_type::kV_TURN_TEAM)}, {u"data", make_obj({{u"team", s(u"")}})}})};
    b->set(u"actions", ensure(acts, items));
  }

  const Value test = field_or(bdy, u"test");
  b->set(u"__tester", truthy(test) ? test : Value());

  Value actions = field_or(bdy, u"actions");
  if (truthy(actions)) {
    Array* const a = as_array(actions);
    if (a == nullptr) return false;
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_action(item)) return false;
    }
  }

  ctx_o->set(u"bdy", bdy);
  return true;
}

}
}
