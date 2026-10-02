#include "lfw/dat_translator/bots/bot_actions.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/constants.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/defines/bot_state_enum.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/game_key.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace dat_translator {
namespace bots {

namespace {

Value sv(const char16_t* s) { return Value(std::u16string(s)); }

double num_or(const Value& v, double fallback) {
  if (std::holds_alternative<std::monostate>(v)) return fallback;
  return to_number(v);
}

Value obj(std::initializer_list<std::pair<const char16_t*, Value>> kv) {
  Object o;
  for (const std::pair<const char16_t*, Value>& p : kv) o.set(std::u16string(p.first), p.second);
  return Value(std::make_shared<Object>(o));
}

Value arr(std::vector<Value> items) { return Value(std::make_shared<Array>(std::move(items))); }

Value keys_or(const Value& keys, std::initializer_list<const char16_t*> fallback) {
  const Array* a = as_array(keys);
  if (a != nullptr && a->size() != 0) return keys;
  std::vector<Value> out;
  for (const char16_t* k : fallback) out.push_back(sv(k));
  return arr(std::move(out));
}

Value override_z(const Value& src, const Value& z) {
  const Object* o = as_object(src);
  if (o == nullptr) return Value();
  Object out;
  for (const std::u16string& k : o->keys()) {
    const Value* p = o->get(k);
    if (p == nullptr) continue;
    out.set(k, k == u"z" ? z : *p);
  }
  return Value(std::make_shared<Object>(out));
}

Value apply_edit(const EditBotAction* fn, const Value& action, CondMaker& cond) {
  if (fn == nullptr) return action;
  Value a = action;
  return (*fn)(a, cond);
}

Value ray_with(double min_x, const Value& max_x, const Value& max_d, bool has_max_d) {
  if (!has_max_d) {
    return obj({{u"x", Value(1.0)}, {u"z", Value(0.0)}, {u"min_x", Value(min_x)}, {u"max_x", max_x}});
  }
  return obj({{u"x", Value(1.0)},
              {u"z", Value(0.0)},
              {u"min_x", Value(min_x)},
              {u"max_x", max_x},
              {u"max_d", max_d}});
}

}

EditBotActionFunc bot_ball_cancelling(const std::u16string& action_id, const Value& desire,
                                      const Value& keys) {
  return [action_id, desire, keys](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"e_ray", arr({obj({{u"x", Value(1.0)},
                                                 {u"z", Value(0.0)},
                                                 {u"reverse", Value(true)}})})},
                           {u"keys", keys_or(keys, {gk::kj})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_ball_continuation(const std::u16string& action_id, const Value& desire,
                                        const Value& mp, const Value& keys) {
  return [action_id, desire, mp, keys](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", mp);
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
                           {u"e_ray", arr({obj({{u"x", Value(1.0)}, {u"z", Value(0.0)}})})},
                           {u"keys", keys_or(keys, {gk::kAttack})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_chasing_action(const std::u16string& action_id, const Value& keys,
                                     const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
                           {u"keys", keys}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_chasing_skill_action(const std::u16string& keys_str, const Value& action_id,
                                           const Value& min_mp, const Value& desire) {
  std::vector<Value> keys;
  keys.push_back(sv(gk::kd));
  const char16_t k1 = keys_str.size() > 1 ? keys_str[1] : u'\0';
  switch (k1) {
    case u'^': keys.push_back(sv(gk::kU)); break;
    case u'v': keys.push_back(sv(gk::kD)); break;
    case u'>': keys.push_back(sv(u"F")); break;
    case u'j': keys.push_back(sv(gk::kj)); break;
    default: break;
  }
  const char16_t k2 = keys_str.size() > 2 ? keys_str[2] : u'\0';
  switch (k2) {
    case u'a': keys.push_back(sv(gk::ka)); break;
    case u'j': keys.push_back(sv(gk::kj)); break;
    default: break;
  }
  const Value keys_value = arr(std::move(keys));
  const std::u16string aid =
      std::holds_alternative<std::monostate>(action_id) ? keys_str : to_string(action_id);
  return [keys_value, aid, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ret = obj({{u"action_id", Value(aid)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
                           {u"keys", keys_value}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_front_test(const std::u16string& action_id, const Value& keys,
                                 const Value& min_mp, const Value& desire, const Value& min_x,
                                 const Value& max_x, const Value& zable) {
  return [action_id, keys, min_mp, desire, min_x, max_x, zable](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ray_1 = ray_with(num_or(min_x, 0.0), max_x, Value(), false);
    std::vector<Value> rays;
    rays.push_back(ray_1);
    const double z = num_or(zable, 0.0);
    if (truthy(zable) && z > 0) {
      rays.push_back(override_z(ray_1, Value(-z)));
      rays.push_back(override_z(ray_1, zable));
    }
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"e_ray", arr(std::move(rays))},
                           {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
                           {u"keys", keys}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_ball_dfa(const Value& min_mp, const Value& desire, const Value& min_x,
                               const Value& max_x, const Value& zable) {
  return bot_front_test(u"d>a", arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}), min_mp,
                        num_or(desire, DESIRE_RATIO_X_2), min_x, max_x, zable);
}

EditBotActionFunc bot_ball_dfj(const Value& min_mp, const Value& desire, const Value& min_x,
                               const Value& max_x, const Value& zable) {
  return bot_front_test(u"d>j", arr({sv(gk::kd), sv(u"F"), sv(gk::kj)}), min_mp,
                        num_or(desire, DESIRE_RATIO_X_2), min_x, max_x, zable);
}

EditBotActionFunc bot_explosion_dua(const Value& min_mp, const Value& desire, const Value& min_x,
                                    const Value& max_x, const Value& z_len) {
  return [min_mp, desire, min_x, max_x, z_len](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", Value(mp_n));
    const double d = lfw::pow(num_or(z_len, 120.0), 2.0);
    const Value ret = obj(
        {{u"action_id", Value(u"d^a")},
         {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
         {u"status", arr({sv(bot_state_enum::kChasing)})},
         {u"e_ray", arr({ray_with(num_or(min_x, -120.0), Value(num_or(max_x, 120.0)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_explosion_duj(const Value& min_mp, const Value& desire, const Value& min_x,
                                    const Value& max_x, const Value& z_len) {
  return [min_mp, desire, min_x, max_x, z_len](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", Value(mp_n));
    const double d = lfw::pow(num_or(z_len, 120.0), 2.0);
    const Value ret = obj(
        {{u"action_id", Value(u"d^j")},
         {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
         {u"status", arr({sv(bot_state_enum::kChasing)})},
         {u"e_ray", arr({ray_with(num_or(min_x, -120.0), Value(num_or(max_x, 120.0)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_idle_action(const std::u16string& action_id, const Value& keys,
                                  const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kIdle)})},
                           {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
                           {u"keys", keys}});
    return apply_edit(fn, ret, cond);
  };
}

Value bot_uppercut_dua(const Value& min_mp, const Value& desire, const Value& min_x,
                       const Value& max_x, const Value& max_d) {
  const double mp_n = num_or(min_mp, 0.0);
  CondMaker cond;
  cond.add(sv(entity_val::kMP), u">=", min_mp);
  const double d = num_or(max_d, 30.0);
  return obj({{u"action_id", Value(u"d^a")},
              {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO_X_3)))},
              {u"status", arr({sv(bot_state_enum::kChasing)})},
              {u"e_ray", arr({ray_with(num_or(min_x, -10.0), Value(num_or(max_x, 120.0)),
                                         Value(d * d), true)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});
}

Value bot_uppercut_duj(const Value& min_mp, const Value& desire, const Value& min_x,
                       const Value& max_x) {
  const double mp_n = num_or(min_mp, 0.0);
  CondMaker cond;
  cond.add(sv(entity_val::kMP), u">=", min_mp);
  return obj({{u"action_id", Value(u"d^j")},
              {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO_X_3)))},
              {u"status", arr({sv(bot_state_enum::kChasing)})},
              {u"e_ray", arr({ray_with(num_or(min_x, -10.0), Value(num_or(max_x, 120.0)),
                                         Value(), false)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)})}});
}

EditBotActionFunc bot_uppercut_dva(const Value& min_mp, const Value& desire, const Value& min_x,
                                   const Value& max_x, const Value& max_d) {
  return [min_mp, desire, min_x, max_x, max_d](const EditBotAction* fn) -> Value {
    const double mp_n = num_or(min_mp, 0.0);
    CondMaker cond;
    cond.add(sv(entity_val::kMP), u">=", min_mp);
    const double d = num_or(max_d, 30.0);
    const Value ret = obj(
        {{u"action_id", Value(u"dva")},
         {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO_X_3)))},
         {u"status", arr({sv(bot_state_enum::kChasing)})},
         {u"e_ray", arr({ray_with(num_or(min_x, -10.0), Value(num_or(max_x, 120.0)),
                                    Value(d * d), true)})},
         {u"expression", Value(cond.done())},
         {u"keys", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)})}});
    return apply_edit(fn, ret, cond);
  };
}

}
}
}
