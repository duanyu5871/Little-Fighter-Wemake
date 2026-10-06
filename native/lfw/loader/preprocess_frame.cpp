#include "lfw/loader/preprocess_frame.h"

#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/cook_frame_indicator_info.h"
#include "lfw/dat_translator/frame_behavior.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/make_buring_smoke.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/loader/preprocess_ball_frame.h"
#include "lfw/loader/preprocess_bdy.h"
#include "lfw/loader/preprocess_itr.h"
#include "lfw/loader/preprocess_next_frame.h"
#include "lfw/loader/preprocess_pic.h"
#include "lfw/loader/resolve_prefab.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/container_help/spread_assign.h"
#include "lfw/utils/js_add.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/read_nums.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace loader {

using dat_translator::make_obj;
using dat_translator::n;
using dat_translator::s;

namespace {

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

Value or_nullish(const Value& v, const Value& fallback) { return is_nullish(v) ? fallback : v; }

// `const { x = 0 } = any`：默认值只在 `undefined` 时生效（`null` 照原样取）。
Value destructure_or(const Value& any, const char16_t* key, const Value& dflt) {
  const Value v = field_or(any, key);
  return is_undefined(v) ? dflt : v;
}

bool state_is(const Value& state, StateEnum st) {
  return strict_equals(state, Value(static_cast<double>(st)));
}

// `v?.length` 的真值：数组给元素数、字符串给码元数、其它类型没有 `length`（同 §61 的 `length_of`）。
bool length_truthy(const Value& v) {
  if (const Array* a = as_array(v)) return a->size() != 0;
  if (const std::u16string* text = std::get_if<std::u16string>(&v)) return !text->empty();
  return false;
}

// `o.key ??= v`：只有 `undefined` / `null` 才写。
void set_if_nullish(Object& o, const char16_t* key, const Value& v) {
  if (is_nullish(field_or(o, key))) o.set(std::u16string(key), v);
}

// `traversal(r, (k, v, o) => …)` 的等价物：对象按 `Object.keys` 序、数组按下标序，
// 其它类型没有键（不遍历）。`cb` 就地改值；返回 false ⇒ 整体失败（TS 那边是抛）。
bool each_entry(Value& r, const std::function<bool(const std::u16string&, Value&)>& cb) {
  // TS 的 `traversal(r, …)`：`r` 是假值就没有键；真值但既不是对象也不是数组时，
  // `Object.keys` 对数字 / 布尔给空表（不遍历），对字符串给下标（回调的 `o[k] = …`
  // 在严格模式下给字符串赋值会抛 ⇒ 失败）。
  if (const std::u16string* const text = std::get_if<std::u16string>(&r)) return text->empty();
  if (Object* const o = as_object(r)) {
    const std::vector<std::u16string> keys = o->keys();
    for (const std::u16string& k : keys) {
      const Value* p = o->get(k);
      if (p == nullptr) continue;
      Value v = *p;
      if (!cb(k, v)) return false;
      o->set(k, v);
    }
    return true;
  }
  if (Array* const a = as_array(r)) {
    for (size_t i = 0; i < a->size(); ++i) {
      if (!cb(to_string(Value(static_cast<double>(i))), a->at(i))) return false;
    }
  }
  return true;
}

// `if (v) o[k] = preprocess_next_frame(v)`。
bool each_next_frame(Value& r) {
  return each_entry(r, [](const std::u16string&, Value& v) {
    if (!truthy(v)) return true;
    return preprocess_next_frame(v);
  });
}

// `frame.bdy?.forEach(v => set_hit_flag(v, HitFlag.AllBoth))`：非对象标量会被 `Object.assign`
// 装箱后当场丢弃（没有效果），`null` / `undefined` 会抛。
bool each_set_hit_flag_both(const Value& v) {
  if (is_nullish(v)) return true;
  const Array* const a = as_array(v);
  if (a == nullptr) return false;
  for (size_t i = 0; i < a->size(); ++i) {
    Value item = a->at(i);
    Object* const o = as_mut(item);
    if (o == nullptr) {
      if (is_nullish(item)) return false;
      continue;
    }
    dat_translator::set_hit_flag(*o, n(static_cast<double>(HitFlag::AllBoth)));
  }
  return true;
}

const std::u16string& breakfall_j_expression() {
  static const std::u16string kText = [] {
    CondMaker cm;
    cm.add(s(entity_val::kHP), u">", n(0));
    cm.and_(s(entity_val::kHitByMagicFlute), u"!=", n(1));
    cm.and_(s(entity_val::kCAUGHT), u"!=", n(1));
    return cm.done();
  }();
  return kText;
}

// `fold_aabb`：`min` / `max` 是 `Math.min` / `Math.max`（`undefined` 走解构默认值、
// `null` 被 `ToNumber` 成 0、越界的字符串同样按 `ToNumber`）。
void fold_aabb(Object& frame, const Value& any, double dzl) {
  const double x1 =
      to_number(destructure_or(any, u"x", n(0.0))) - to_number(field_or(frame, u"centerx"));
  const double x2 = to_number(js_add(Value(x1), destructure_or(any, u"w", n(0.0))));
  const Value z = destructure_or(any, u"z", n(-dzl / 2));
  const double z1 = to_number(z);
  const double z2 = to_number(js_add(z, destructure_or(any, u"l", n(dzl))));

  frame.set(u"__aabb_x1",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_x1"), Value(x1))), x1)));
  frame.set(u"__aabb_x2",
            Value(max(to_number(or_nullish(field_or(frame, u"__aabb_x2"), Value(x2))), x2)));
  frame.set(u"__aabb_z1",
            Value(min(to_number(or_nullish(field_or(frame, u"__aabb_z1"), Value(z1))), z1)));
  frame.set(u"__aabb_z2",
            Value(max(to_number(or_nullish(field_or(frame, u"__aabb_z2"), Value(z2))), z2)));
}

// `ensure(output, item)`：`output` 不是假值又不是数组时 TS 的 `output.push` 不是函数 ⇒ 抛。
bool ensure_or_fail(Value& output, const Value& item, Value& out) {
  if (truthy(output) && as_array(output) == nullptr) return false;
  out = ensure(output, item);
  return true;
}

}

bool preprocess_frame(Value& ctx, std::u16string& error) {
  Object* const ctx_o = as_object(ctx);
  if (ctx_o == nullptr) return false;

  const Value data = field_or(ctx, u"data");
  const Value raw_frame = field_or(ctx, u"frame");

  const ResolvePrefabResult merged = resolve_prefab(raw_frame, field_or(data, u"frame_prefabs"));
  if (!merged.ok) {
    const std::u16string who =
        to_string(field_or(data, u"id")) + u":" + to_string(field_or(raw_frame, u"id"));
    error = prefab_error_message(u"preprocess_frame", who, u"frame", merged);
    return false;
  }
  Value frame = merged.value;
  Object* const f = as_mut(frame);
  // TS 在严格模式下给标量挂属性会抛；端口一律按失败处理（同 `preprocess_bdy` / `preprocess_itr`）。
  if (f == nullptr) return false;

  // TS: `if (data.processed != false) {}` —— **松散**比较，`0` / `""` 也算“处理过”。
  if (!equals(field_or(data, u"processed"), Value(false))) {
  } else if (entity::is_ball_data(data)) {
    // 注意 TS 传的是**当前 ctx**（`ctx.frame` 还是原始帧），而返回的是 `merged.value`。
    preprocess_ball_frame(ctx);
  } else if (entity::is_weapon_data(data)) {
    Object* const d = as_mut(data);
    if (d == nullptr) return false;
    Value indexes = field_or(data, u"indexes");
    if (!truthy(indexes)) {
      indexes = Value(std::make_shared<Object>(Object()));
      d->set(u"indexes", indexes);
    }
    Object* const ix = as_mut(indexes);

    Value in_the_skys = field_or(indexes, u"in_the_skys");
    Value throwings = field_or(indexes, u"throwings");
    Value on_hands = field_or(indexes, u"on_hands");
    if (!truthy(in_the_skys)) in_the_skys = Value(std::make_shared<Array>(Array()));
    if (!truthy(throwings)) throwings = Value(std::make_shared<Array>(Array()));
    if (!truthy(on_hands)) on_hands = Value(std::make_shared<Array>(Array()));
    Array* const skys = as_array(in_the_skys);
    Array* const thrs = as_array(throwings);
    Array* const hands = as_array(on_hands);

    const Value frame_id = field_or(frame, u"id");
    const Value state = field_or(frame, u"state");
    const auto push_id = [&](Array* a) {
      if (a == nullptr) return false;  // 已有的值不是数组 ⇒ TS 的 `push` 抛
      a->push_back(frame_id);
      return true;
    };
    if (state_is(state, StateEnum::Weapon_InTheSky)) {
      if (!push_id(skys)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;
    } else if (state_is(state, StateEnum::Weapon_Rebounding) ||
               state_is(state, StateEnum::HeavyWeapon_JustOnGround)) {
      f->set(u"itr", Value());
    } else if (state_is(state, StateEnum::Weapon_Throwing)) {
      if (!push_id(thrs)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;
    } else if (state_is(state, StateEnum::HeavyWeapon_InTheSky)) {
      if (!push_id(skys)) return false;
      if (!push_id(thrs)) return false;
      if (!each_set_hit_flag_both(field_or(frame, u"bdy"))) return false;
    } else if (state_is(state, StateEnum::Weapon_OnHand) ||
               state_is(state, StateEnum::HeavyWeapon_OnHand)) {
      if (!push_id(hands)) return false;
    }
    // TS: `if (in_the_skys.length) data.indexes.in_the_skys = in_the_skys` —— 已有的数组是就地
    // 改的（这一句等于自赋值），只有新建的才需要挂回去；非数组的已有值 `.length` 可能非零，
    // 那时也只是一次自赋值（无效果）。
    if (skys != nullptr && !skys->empty()) ix->set(u"in_the_skys", in_the_skys);
    if (thrs != nullptr && !thrs->empty()) ix->set(u"throwings", throwings);
    if (hands != nullptr && !hands->empty()) ix->set(u"on_hands", on_hands);
  }

  if (entity::is_fighter_data(data)) {
    const Value state = field_or(frame, u"state");
    if (!(state_is(state, StateEnum::Falling) || state_is(state, StateEnum::Caught) ||
          state_is(state, StateEnum::Injured) || state_is(state, StateEnum::Frozen) ||
          state_is(state, StateEnum::Burning))) {
      set_if_nullish(*f, u"stat_recover", n(1));
    }
    if (state_is(state, StateEnum::Standing) || state_is(state, StateEnum::Walking) ||
        state_is(state, StateEnum::Running) || state_is(state, StateEnum::Jump) ||
        state_is(state, StateEnum::Dash) || state_is(state, StateEnum::Lying) ||
        state_is(state, StateEnum::Rowing)) {
      set_if_nullish(*f, u"toughness_recover", n(1));
    }
  }

  const Value frame_pic = field_or(frame, u"pic");
  set_if_nullish(*f, u"width", or_nullish(field_or(frame_pic, u"w"), n(0.0)));
  set_if_nullish(*f, u"height", or_nullish(field_or(frame_pic, u"h"), n(0.0)));

  if (!dat_translator::cook_frame_indicator_info(frame)) return false;
  if (entity::is_weapon_data(data) || entity::is_ball_data(data)) {
    dat_translator::make_frame_behavior(frame, to_string(field_or(data, u"id")));
  }

  // `frame.sound` 在这里只往 `jobs` 里塞加载任务（`lfw.sounds.load`），端口不落地
  // （同 §64.1 的既有约定：`A_SOUND` 只校验 path 可迭代）。

  const Value seqs = field_or(frame, u"seqs");
  if (truthy(seqs)) {
    // TS 先建 `__seq_map` 再遍历：遍历中途抛（例如 `seqs` 是字符串）时 map 已经挂上了。
    Value seq_map = Value(std::make_shared<Object>(Object()));
    f->set(u"__seq_map", seq_map);
    Value seqs_local = seqs;
    const bool ok = each_entry(seqs_local, [&](const std::u16string& key, Value& v) {
      if (!truthy(v)) return true;
      if (!preprocess_next_frame(v)) return false;
      as_mut(seq_map)->set(key, v);
      return true;
    });
    if (!ok) return false;
  }

  // 按键受身的限制（见 TS 的注释）：`Falling` 帧的 `hit.j` 里 id 为 100 / 108 的项换成新表达式。
  if (state_is(field_or(frame, u"state"), StateEnum::Falling)) {
    const Value j = field_or(field_or(frame, u"hit"), u"j");
    if (truthy(j)) {
      const Value expr = Value(breakfall_j_expression());
      const auto edit = [&](const Value& nf) {
        Object* const o = as_mut(nf);
        if (o == nullptr) return;
        const std::u16string id = to_string(field_or(nf, u"id"));
        if (id == u"100" || id == u"108") o->set(u"expression", expr);
      };
      Value list = j;
      if (const Array* const a = as_array(list)) {
        for (size_t i = 0; i < a->size(); ++i) edit(a->at(i));
      } else {
        edit(list);
      }
    }
  }

  Value hit_map = field_or(frame, u"hit");
  if (!each_next_frame(hit_map)) return false;
  Value hold_map = field_or(frame, u"hold");
  if (!each_next_frame(hold_map)) return false;
  Value key_down_map = field_or(frame, u"key_down");
  if (!each_next_frame(key_down_map)) return false;
  Value key_up_map = field_or(frame, u"key_up");
  if (!each_next_frame(key_up_map)) return false;

  for (const char16_t* const key : {u"next", u"on_dead", u"on_exhaustion", u"on_landing"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key), nf);
  }

  if (!truthy(field_or(frame, u"on_x_restrict")) &&
      equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball))) &&
      (length_truthy(field_or(frame, u"itr")) || length_truthy(field_or(frame, u"bdy"))) &&
      // 注意这三处是**松散**比较（`"3000"` 也算），与上面 `Falling` 的严格比较不同。
      (equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_Flying))) ||
       equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_3005))) ||
       equals(field_or(frame, u"state"), n(static_cast<double>(StateEnum::Ball_3006))))) {
    // 拥有 itr 或 bdy 的 ball 在 X / Y 轴被阻，破之。`data.base.hit_sounds`：`base` 是
    // `null` / `undefined` 时 TS 会抛。
    const Value base = field_or(data, u"base");
    if (is_nullish(base)) return false;
    const Value sounds = field_or(base, u"hit_sounds");
    f->set(u"on_x_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));
    f->set(u"on_y_restrict", make_obj({{u"id", s(u"20")}, {u"sound", sounds}}));
  }

  for (const char16_t* const key : {u"on_restrict", u"on_x_restrict", u"on_y_restrict",
                                    u"on_z_restrict"}) {
    Value nf = field_or(frame, key);
    if (!truthy(nf)) continue;
    if (!preprocess_next_frame(nf)) return false;
    f->set(std::u16string(key), nf);
  }

  for (const char16_t* const key : {u"bdy", u"itr"}) {
    const bool is_bdy = std::u16string(key) == u"bdy";
    Value list = field_or(frame, key);
    if (is_nullish(list)) continue;
    Array* const a = as_array(list);
    if (a == nullptr) return false;  // TS 的 `?.forEach` 对非数组会抛
    const size_t count = a->size();
    for (size_t i = 0; i < count; ++i) {
      // TS 的 `{ ...ctx, bdy: n, index: i }` —— 用的是**当前的** ctx（`frame` 是原始帧）。
      Value sub = spread_assign(ctx, Value());
      as_mut(sub)->set(std::u16string(key), a->at(i));
      if (!(is_bdy ? preprocess_bdy(sub, error) : preprocess_itr(sub, error))) return false;
      const Value item = field_or(sub, key);
      a->at(i) = item;
      if (!truthy(field_or(item, u"on_hit_ground"))) continue;
      const char16_t* const bucket_key = is_bdy ? u"__hit_ground_bdys" : u"__hit_ground_itrs";
      Value bucket = field_or(frame, bucket_key);
      if (!truthy(bucket)) {
        bucket = Value(std::make_shared<Array>(Array()));
        f->set(std::u16string(bucket_key), bucket);
      }
      Array* const bucket_array = as_array(bucket);
      if (bucket_array == nullptr) return false;  // TS 的 `?.push` 对非数组会抛
      bucket_array->push_back(item);
    }
  }

  const Value state = field_or(frame, u"state");
  if (state_is(state, StateEnum::Burning) &&
      !equals(field_or(data, u"id"), s(oid::kJulianBall2))) {
    Value opoint = field_or(frame, u"opoint");
    Value next;
    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(1), next)) return false;
    f->set(u"opoint", next);
  } else if (state_is(state, StateEnum::BurnRun)) {
    Value opoint = field_or(frame, u"opoint");
    Value next;
    if (!ensure_or_fail(opoint, dat_translator::make_buring_smoke(2), next)) return false;
    f->set(u"opoint", next);
  }
  // `frame.opoint?.forEach(preprocess_opoint)`：编译结果（`__gen_*`）装不进 `Value`
  // （同 `__tester` / `__judger`），端口不落地 —— 宿主按 `IEntityHost::gen_or` 现解析。

  const Value center = field_or(frame, u"center");
  if (truthy(center)) {
    std::vector<Value> nums;
    if (!read_nums(center, 2, Value(), nums, &error)) return false;
    f->set(u"centerx", nums[0]);
    f->set(u"centery", nums[1]);
  }

  f->set(u"pic", preprocess_frame_pic(frame));
  Value pics = field_or(frame, u"pics");
  if (!is_nullish(pics)) {
    Array* const a = as_array(pics);
    if (a == nullptr) return false;  // TS 的 `?.forEach` 对非数组会抛
    for (size_t i = 0; i < a->size(); ++i) {
      Value pic = a->at(i);
      a->at(i) = preprocess_pic(pic);
    }
  }

  if (is_undefined(field_or(frame, u"landable"))) {
    f->set(u"landable",
           Value(strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball)))
                     ? 0.0
                     : 1.0));
  }

  if (strict_equals(field_or(frame, u"behavior"),
                    n(static_cast<double>(FrameBehavior::Boomerang))) &&
      is_undefined(field_or(frame, u"facing")) &&
      strict_equals(field_or(data, u"type"), n(static_cast<double>(EntityEnum::Ball)))) {
    f->set(u"facing", n(static_cast<double>(FacingFlag::VX)));
  }

  const double dzl = defines::num(u"Defines.DAFUALT_QUBE_LENGTH");
  Value bdy_list = field_or(frame, u"bdy");
  if (is_nullish(bdy_list)) {
  } else if (Array* const a = as_array(bdy_list)) {
    for (size_t i = 0; i < a->size(); ++i) fold_aabb(*f, a->at(i), dzl);
  } else {
    return false;  // TS 的 `?.forEach` 对非数组会抛
  }
  Value itr_list = field_or(frame, u"itr");
  if (is_nullish(itr_list)) {
  } else if (Array* const a = as_array(itr_list)) {
    for (size_t i = 0; i < a->size(); ++i) fold_aabb(*f, a->at(i), dzl);
  } else {
    return false;
  }

  ctx_o->set(u"frame", frame);
  return true;
}

}
}
