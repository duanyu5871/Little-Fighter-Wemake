#include "lfw/loader/preprocess_entity_data.h"

#include <memory>
#include <string>
#include <variant>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/entity_data.h"
#include "lfw/dat_translator/make_ball_special.h"
#include "lfw/dat_translator/make_fighter_special.h"
#include "lfw/dat_translator/make_weapon_special.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/entity_val.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/loader/preprocess_bdy.h"
#include "lfw/loader/preprocess_bot_data.h"
#include "lfw/loader/preprocess_frame.h"
#include "lfw/loader/preprocess_itr.h"
#include "lfw/loader/preprocess_next_frame.h"
#include "lfw/loader/preprocess_pic.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/container_help/spread_assign.h"
#include "lfw/utils/container_help/traversal.h"
#include "lfw/utils/math/base.h"
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

// `v?.length` 的**值**版本（`null` / `undefined` 被 `?.` 短路成 `undefined`）。
Value value_length(const Value& v) {
  if (is_nullish(v)) return Value();
  if (const Array* const a = as_array(v)) return Value(static_cast<double>(a->size()));
  if (const std::u16string* const text = std::get_if<std::u16string>(&v))
    return Value(static_cast<double>(text->size()));
  if (as_object(v) != nullptr) return field_or(v, u"length");
  return Value();
}

// `jobs.push(...)`：TS 里 `jobs` 没有可调用的 `push` 就抛，端口按「是数组才算有 push」处理。
bool push_job(const Value& jobs) { return as_array(jobs) != nullptr; }

// `data.base.<key>?.forEach(...)`：端口不落地加载任务，只保留失败面 —— 真值但不是数组时
// `forEach` 不是函数（TS 抛），`null` / `undefined` 才被 `?.` 短路。
bool each_sound(const Value& base, const char16_t* key) {
  const Value list = field_or(base, key);
  if (is_nullish(list)) return true;
  return as_array(list) != nullptr;
}

// `itr.test ??= weapon_on_hand_dont_hit_falling_guy`（TS 那边是模块级的 `new CondMaker()….done()`）。
Value weapon_on_hand_dont_hit_falling_guy() {
  CondMaker cond;
  cond.add(s(collision_val::kVFALLING), u"==", n(0));
  return Value(cond.done());
}

// `data.pre_hitkeys ??= { ja: { reset_keys: 1, transfrom_to_another: 1, expression: … } }`
Value default_pre_hitkeys() {
  CondMaker cond;
  cond.add(s(entity_val::kTransformListSize), u"==", n(2))
      .and_(s(entity_val::kIsOnGround), u"==", n(1))
      .and_(s(entity_val::kTransformIndex), u"==", n(1));
  return make_obj({{u"ja", make_obj({{u"reset_keys", n(1)},
                                     {u"transfrom_to_another", n(1)},
                                     {u"expression", Value(cond.done())}})}});
}

// `if (data.<key>) { const map = new Map(); traversal(…); if (map.size) data.__<x>_map = map; }`：
// TS 的 `Map` 在这里只当「键序 = 首次插入序、重复键不增 size」的字典用，端口用普通对象
// （`renderValue` 对 `Map` 与对象输出同形）。
bool build_hitkeys_map(Value& data, const char16_t* key, const char16_t* out_key) {
  const Value holder = field_or(data, key);
  Value map = Value(std::make_shared<Object>(Object()));
  bool any = false;
  // TS 的 `traversal` 对字符串拿 `Object.keys`（下标），值是那一个字符。回调里 `k.length < 2`
  // （下标最多两位起）与 `k[0] == k[1]`（"11" 这种）是提前 return，**没有**走到
  // `map.set(k, o[k] = nf)`，所以字符串不一定失败 —— 这一点和 `traversal_write` 的「非空字符串
  // 直接失败」不同（那个前提是回调总会写回），只能在这儿自己判。
  if (const std::u16string* const text = std::get_if<std::u16string>(&holder)) {
    for (size_t i = 0; i < text->size(); ++i) {
      const std::u16string k = number_to_string(static_cast<double>(i));
      if (k.size() < 2 || k[0] == k[1]) continue;
      Value v(std::u16string(1, (*text)[i]));
      if (!truthy(v)) continue;
      if (!preprocess_next_frame(v)) return false;
      return false;   // `o[k] = nf` 给字符串赋值 ⇒ TS 抛
    }
    return true;
  }
  Value local = holder;
  const bool ok = traversal_write(local, [&](const std::u16string& k, Value& v) {
    if (!truthy(v)) return true;
    if (k.size() < 2) return true;
    if (k[0] == k[1]) return true;
    // TS 的调用点是 `map.set(k, o[k] = nf)`，`nf` 的写回由 `traversal_write` 负责。
    if (!preprocess_next_frame(v)) return false;
    as_mut(map)->set(k, v);
    any = true;
    return true;
  });
  if (!ok) return false;
  if (any) as_mut(data)->set(std::u16string(out_key), map);
  return true;
}

}

bool preprocess_entity_data(Value& ctx, std::u16string& error) {
  if (is_nullish(ctx)) return false;   // `const { lfw, data, jobs, errors } = ctx`
  Value data = field_or(ctx, u"data");
  if (is_nullish(data)) return false;  // `data.processed` 抛

  // `if (data.processed != false) { }`：TS 里是空分支（`undefined != false` 为真），没有副作用。
  if (entity::is_ball_data(data)) {
    dat_translator::make_ball_special(data);
  } else if (entity::is_weapon_data(data)) {
    dat_translator::make_weapon_special(data);
  } else if (entity::is_fighter_data(data)) {
    if (is_nullish(field_or(data, u"pre_hitkeys")))
      as_mut(data)->set(u"pre_hitkeys", default_pre_hitkeys());
    // TS 丢掉 `make_fighter_special` 的返回值（它可能返回新对象），端口照做。
    (void)dat_translator::make_fighter_special(data);
  }

  Value itr_prefabs = field_or(data, u"itr_prefabs");
  {
    const bool ok = traversal_write(itr_prefabs, [&](const std::u16string&, Value& itr) {
      if (!truthy(itr)) return true;
      if (entity::is_weapon_data(data) && is_nullish(field_or(itr, u"test"))) {
        Object* const io = as_mut(itr);
        if (io == nullptr) return false;   // 给标量挂 `test` ⇒ TS 抛
        io->set(u"test", weapon_on_hand_dont_hit_falling_guy());
      }
      Value sub = spread_assign(make_obj({{u"itr", itr}}), ctx);
      if (!preprocess_itr(sub, error)) return false;
      itr = field_or(sub, u"itr");
      return true;
    });
    if (!ok) return false;
  }

  Value bdy_prefabs = field_or(data, u"bdy_prefabs");
  {
    const bool ok = traversal_write(bdy_prefabs, [&](const std::u16string&, Value& bdy) {
      if (!truthy(bdy)) return true;
      Value sub = spread_assign(make_obj({{u"bdy", bdy}}), ctx);
      if (!preprocess_bdy(sub, error)) return false;
      bdy = field_or(sub, u"bdy");
      return true;
    });
    if (!ok) return false;
  }

  const Value lfw = field_or(ctx, u"lfw");
  if (is_nullish(lfw)) return false;   // `const { images, sounds } = lfw` 抛
  const Value jobs = field_or(ctx, u"jobs");
  const Value base = field_or(data, u"base");
  if (is_nullish(base)) return false;  // `const { small, head } = data.base` 抛

  // `if (is_non_blank_str(small)) jobs.push(images.load_img(small, small))`：端口不落地加载任务，
  // 只保留「`jobs` 不是数组 ⇒ `jobs.push` 抛」。
  if (is_non_blank_str(field_or(base, u"small")) && !push_job(jobs)) return false;
  if (is_non_blank_str(field_or(base, u"head")) && !push_job(jobs)) return false;
  if (!each_sound(base, u"dead_sounds")) return false;
  if (!each_sound(base, u"drop_sounds")) return false;
  if (!each_sound(base, u"hit_sounds")) return false;

  if (truthy(field_or(data, u"pre_hitkeys")) &&
      !build_hitkeys_map(data, u"pre_hitkeys", u"__pre_hitkeys_map"))
    return false;
  if (truthy(field_or(data, u"post_hitkeys")) &&
      !build_hitkeys_map(data, u"post_hitkeys", u"__post_hitkeys_map"))
    return false;

  if (truthy(field_or(data, u"on_dead"))) {
    Value nf = field_or(data, u"on_dead");
    if (!preprocess_next_frame(nf)) return false;
    as_mut(data)->set(u"on_dead", nf);
  }
  if (truthy(field_or(data, u"on_exhaustion"))) {
    Value nf = field_or(data, u"on_exhaustion");
    if (!preprocess_next_frame(nf)) return false;
    as_mut(data)->set(u"on_exhaustion", nf);
  }

  const Value files = field_or(base, u"files");
  const Value portraits = field_or(base, u"portraits");
  const Value frames = field_or(data, u"frames");

  // `traversal(files, (_, v) => jobs.push(images.load_by_pic_info(v)))`：端口不落地加载任务，
  // 只保留「每个键都会 push」这个失败面（字符串的 `Object.keys` 是下标）。
  {
    Value holder = files;
    bool push_failed = false;
    traversal(holder, [&](const std::u16string&, Value&) {
      if (!push_job(jobs)) push_failed = true;
    });
    if (push_failed) return false;
  }

  // `if (jobs.length) await Promise.all(jobs)`：端口不落地加载任务，只保留「`jobs` 缺失 ⇒
  // `jobs.length` 抛」与「`jobs` 不可迭代 ⇒ `Promise.all` 抛」。
  if (is_nullish(jobs)) return false;
  if (truthy(value_length(jobs)) && as_array(jobs) == nullptr && !is_str(jobs)) return false;

  {
    Value holder = portraits;
    const bool ok = traversal_write(holder, [&](const std::u16string&, Value& v) {
      // TS 的调用点是 `o[k] = preprocess_pic(lfw, data, v)`；`preprocess_pic` 就地改并返回同一个
      // 对象（非对象原样返回），写回是恒等操作。
      v = preprocess_pic(v);
      return true;
    });
    if (!ok) return false;
  }

  {
    Value holder = frames;
    const bool ok = traversal_write(holder, [&](const std::u16string&, Value& frame) {
      // TS 的调用点是 `o[fid] = preprocess_frame({ ...ctx, frame })`。
      const Value raw_frame = frame;
      Value sub = spread_assign(ctx, make_obj({{u"frame", raw_frame}}));
      if (!preprocess_frame(sub, error)) return false;
      frame = field_or(sub, u"frame");
      // `const pics = frame.pics?.length;` 读的是**回调参数**（`preprocess_frame` 的返回值不会
      // 改到它）；`frame` 是 prefab 键时 `pics` 就是 `undefined`。
      const Value pics = value_length(field_or(raw_frame, u"pics"));
      if (truthy(pics)) {
        const Value prev = field_or(data, u"__pics");
        const double a = to_number(pics);
        const double b = to_number(truthy(prev) ? prev : Value(0.0));
        as_mut(data)->set(u"__pics", Value(max(a, b)));
      }
      return true;
    });
    if (!ok) return false;
  }

  if (truthy(field_or(base, u"bot"))) {
    Value bot = field_or(base, u"bot");
    if (!preprocess_bot_data(bot)) return false;
    as_mut(base)->set(u"bot", bot);
  } else {
    dat_translator::make_entity_special(data);
  }
  as_mut(data)->set(u"processed", Value(true));

  // `if (errors.length) Ditto.warn(errors)`：端口不落地 `Ditto`，也还没有 `errors` 的收集
  // （见 DESIGN §66），只保留「`errors` 缺失 ⇒ `errors.length` 抛」。
  if (is_nullish(field_or(ctx, u"errors"))) return false;
  return true;
}

}
}
