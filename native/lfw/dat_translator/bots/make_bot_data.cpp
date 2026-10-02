#include "lfw/dat_translator/bots/make_bot_data.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bot_actions.h"
#include "lfw/dat_translator/bots/frames.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/math/range.h"

namespace lfw {
namespace dat_translator {
namespace bots {

namespace {

Value sv(const char16_t* s) { return Value(std::u16string(s)); }

Value num(double d) { return Value(d); }

Value arr(std::vector<Value> items) { return Value(std::make_shared<Array>(std::move(items))); }

Value obj(std::initializer_list<std::pair<const char16_t*, Value>> kv) {
  Object o;
  for (const std::pair<const char16_t*, Value>& p : kv) o.set(std::u16string(p.first), p.second);
  return Value(std::make_shared<Object>(o));
}

Value field_of(const Value& v, const char16_t* key) {
  const Object* o = as_object(v);
  const Value* p = o != nullptr ? o->get(std::u16string(key)) : nullptr;
  return p != nullptr ? *p : Value();
}

Value frames_field(const char16_t* name) { return field_of(frames_object(), name); }

Value concat(std::initializer_list<Value> arrays) {
  std::vector<Value> out;
  for (const Value& a : arrays) {
    const Array* src = as_array(a);
    if (src == nullptr) continue;
    for (size_t i = 0; i < src->size(); ++i) out.push_back(src->at(i));
  }
  return arr(std::move(out));
}

Value range_array(double from, double to) {
  std::vector<Value> out;
  const std::optional<std::vector<double>> r = range(from, to);
  if (r.has_value()) {
    for (double d : *r) out.push_back(Value(d));
  }
  return arr(std::move(out));
}

const EditBotAction* edit_return_a() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    return action;
  };
  return &e;
}

const EditBotAction* edit_monk_keys() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"keys", arr({sv(gk::kDefend), sv(u"F"), sv(gk::kAttack)}));
    }
    return action;
  };
  return &e;
}

}

BotMaker make_bot_data_bat() {
  BotMaker m(oid::kBat);
  m.set_actions({
      as_action(bot_ball_dfa(num(25), Value(), num(80))),
      as_action(bot_ball_dfj(num(50), Value(), num(0), num(120))),
      as_action(bot_chasing_skill_action(u"d^j", Value(), num(200), num(0.05))),
      as_action(bot_chasing_skill_action(u"dva", Value())),
  });
  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}), arr({sv(u"dva")}));
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings"),
                       frames_field(u"runnings")}),
               arr({sv(u"d^j"), sv(u"d>j"), sv(u"d>a")}));
  return m;
}

BotMaker make_bot_data_hunter() {
  BotMaker m(oid::kHunter);
  m.set_dataset(obj({{u"w_atk_m_x", num(79)},
                     {u"w_atk_r_x", num(200)},
                     {u"w_atk_x", num(200)},
                     {u"j_atk_x", num(200)}}));
  return m;
}

BotMaker make_bot_data_jan() {
  BotMaker m(oid::kJan);
  m.set_actions({
      as_action(bot_explosion_dua(num(150), Value(), num(50), num(400), num(160000))),
      as_action(bot_explosion_duj(num(200), Value(), num(0), num(10000))(edit_return_a())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^a"), sv(u"d^j")}));
  return m;
}

BotMaker make_bot_data_knight() {
  BotMaker m(oid::kKnight);
  m.set_dataset(obj({{u"w_atk_x", num(90)},
                     {u"j_atk_x", num(90)},
                     {u"d_atk_max_x", num(200)},
                     {u"r_atk_x", num(150)}}));
  return m;
}

BotMaker make_bot_data_monk() {
  BotMaker m(oid::kMonk);
  m.set_actions({
      as_action(bot_ball_dfa(num(100), Value(), num(0), num(400))),
      as_action(bot_ball_dfa(num(100), num(0.5), num(0), num(400))(edit_monk_keys())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>a")}));
  m.set_frames(range_array(240.0, 248.0), arr({sv(u"d>a+d>a")}));
  return m;
}

void register_all_bots() {
  BotMaker::register_maker(oid::kBat, make_bot_data_bat);
  BotMaker::register_maker(oid::kHunter, make_bot_data_hunter);
  BotMaker::register_maker(oid::kJan, make_bot_data_jan);
  BotMaker::register_maker(oid::kKnight, make_bot_data_knight);
  BotMaker::register_maker(oid::kMonk, make_bot_data_monk);
}

}
}
}
