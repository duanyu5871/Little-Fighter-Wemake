#include "lfw/dat_translator/bots/make_bot_data.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/dat_translator/bots/bot_actions.h"
#include "lfw/dat_translator/bots/constants.h"
#include "lfw/dat_translator/bots/frames.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/defines/bot_state_enum.h"
#include "lfw/defines/bot_val.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/math/probability.h"
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

Value ray_with_z(const Value& src, double z) {
  const Object* o = as_object(src);
  if (o == nullptr) return Value();
  Object out;
  for (const std::u16string& k : o->keys()) {
    const Value* p = o->get(k);
    if (p == nullptr) continue;
    out.set(k, k == u"z" ? Value(z) : *p);
  }
  return Value(std::make_shared<Object>(out));
}

void set_rays_max_d(Value& action, double v) {
  Object* o = as_object(action);
  if (o == nullptr) return;
  const Value* rays_v = o->get(u"e_ray");
  Array* rays = rays_v != nullptr ? const_cast<Array*>(as_array(*rays_v)) : nullptr;
  if (rays == nullptr) return;
  for (size_t i = 0; i < rays->size(); ++i) {
    Object* r = as_object(rays->at(i));
    if (r != nullptr) r->set(u"max_d", Value(v));
  }
}

const EditBotAction* edit_davis_dva_j() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"action_id", sv(u"dva+j"));
      o->set(u"keys", arr({sv(gk::kj)}));
      cond.and_(sv(bot_val::kEnemyY), u">", Value(0.0));
      o->set(u"expression", Value(cond.done()));
    }
    return action;
  };
  return &e;
}

const EditBotAction* edit_davis_dj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"action_id", sv(u"d^j"));
      o->set(u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)}));
    }
    return action;
  };
  return &e;
}

const EditBotAction* edit_davis_dj_a() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"action_id", sv(u"d^j+a"));
      o->set(u"keys", arr({sv(gk::ka)}));
    }
    return action;
  };
  return &e;
}

const EditBotAction* edit_louis_dja() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    cond.and_(sv(entity_val::kHP_P), u"<", Value(33.0));
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

const EditBotAction* edit_sorcerer_status() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"status", arr({sv(bot_state_enum::kIdle), sv(bot_state_enum::kChasing),
                              sv(bot_state_enum::kAvoiding)}));
      cond.and_(sv(entity_val::kHpRecoverable), u">=", Value(50.0));
      cond.and_(sv(bot_val::kSafe), u"==", Value(1.0));
      o->set(u"expression", Value(cond.done()));
    }
    return action;
  };
  return &e;
}

const EditBotAction* edit_mark_rays_max_d() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    set_rays_max_d(action, 1600.0);
    return action;
  };
  return &e;
}

const EditBotAction* edit_mark_dfj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    set_rays_max_d(action, 1600.0);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

const EditBotAction* edit_mark_cancel() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    Object* o = as_object(action);
    if (o == nullptr) return action;
    o->set(u"action_id", sv(u"cancel_d>j"));
    const Value* rays_v = o->get(u"e_ray");
    Array* rays = rays_v != nullptr ? const_cast<Array*>(as_array(*rays_v)) : nullptr;
    if (rays != nullptr && rays->size() > 0) {
      Object* ray = as_object(rays->at(0));
      if (ray != nullptr) ray->set(u"reverse", Value(true));
      const Value first = rays->at(0);
      rays->push_back(ray_with_z(first, 0.2));
      rays->push_back(ray_with_z(first, -0.2));
    }
    cond.or_(sv(bot_val::kEnemyDiffX), u"<", Value(-100.0));
    o->set(u"expression", Value(cond.done()));
    o->set(u"keys", arr({sv(gk::kJump)}));
    return action;
  };
  return &e;
}

Array* rays_of(Value& action) {
  Object* o = as_object(action);
  if (o == nullptr) return nullptr;
  const Value* rays_v = o->get(u"e_ray");
  return rays_v != nullptr ? const_cast<Array*>(as_array(*rays_v)) : nullptr;
}

void push_ray_with_z(Value& action, double z) {
  Array* rays = rays_of(action);
  if (rays == nullptr || rays->size() == 0) return;
  const Value first = rays->at(0);
  rays->push_back(ray_with_z(first, z));
}

void set_ray0_max_d(Value& action, double v) {
  Array* rays = rays_of(action);
  if (rays == nullptr || rays->size() == 0) return;
  Object* r = as_object(rays->at(0));
  if (r != nullptr) r->set(u"max_d", Value(v));
}

void set_rays_reverse(Value& action) {
  Array* rays = rays_of(action);
  if (rays == nullptr) return;
  for (size_t i = 0; i < rays->size(); ++i) {
    Object* r = as_object(rays->at(i));
    if (r != nullptr) r->set(u"reverse", Value(true));
  }
}

void set_ray0_z(Value& action, double z) {
  Array* rays = rays_of(action);
  if (rays == nullptr || rays->size() == 0) return;
  Object* r = as_object(rays->at(0));
  if (r != nullptr) r->set(u"z", Value(z));
}

void set_rays_reverse_and_keys(Value& action, const Value& keys) {
  set_rays_reverse(action);
  Object* o = as_object(action);
  if (o != nullptr) o->set(u"keys", keys);
}

const EditBotAction* edit_henry_dja(int index) {
  static const EditBotAction e1 = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"action_id", sv(u"dja_1"));
      o->set(u"keys", arr({sv(gk::kd), sv(gk::kj), sv(gk::ka)}));
    }
    set_ray0_max_d(action, 40000.0);
    return action;
  };
  static const EditBotAction e2 = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"action_id", sv(u"dja_2"));
      o->set(u"keys", arr({sv(gk::kd), sv(gk::kj), sv(gk::ka)}));
    }
    set_ray0_max_d(action, 160000.0);
    return action;
  };
  return index == 1 ? &e1 : &e2;
}

const EditBotAction* edit_firzen_dj2() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"action_id", sv(u"d^j_2"));
    return action;
  };
  return &e;
}

const EditBotAction* edit_firen_dfj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    push_ray_with_z(action, 0.2);
    push_ray_with_z(action, -0.2);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

const EditBotAction* edit_firen_dvj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"action_id", sv(u"dvj"));
    push_ray_with_z(action, 0.05);
    push_ray_with_z(action, -0.1);
    if (o != nullptr) {
      o->set(u"keys", arr({sv(gk::kDefend), sv(gk::kDown), sv(gk::kJump)}));
    }
    return action;
  };
  return &e;
}

const EditBotAction* edit_firen_cancel_dvj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"action_id", sv(u"cancel_dvj"));
    Array* rays = rays_of(action);
    if (rays != nullptr && rays->size() > 0) {
      Object* r = as_object(rays->at(0));
      if (r != nullptr) r->set(u"reverse", Value(true));
    }
    push_ray_with_z(action, 0.05);
    push_ray_with_z(action, -0.05);
    if (o != nullptr) o->set(u"keys", arr({sv(gk::kJump)}));
    return action;
  };
  return &e;
}

const EditBotAction* edit_woody_dfa() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    (void)cond;
    set_ray0_z(action, 0.1);
    push_ray_with_z(action, -0.1);
    return action;
  };
  return &e;
}

const EditBotAction* edit_woody_dfj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    push_ray_with_z(action, 0.2);
    push_ray_with_z(action, -0.2);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

const EditBotAction* edit_woody_c_dj() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"action_id", sv(u"c_d>j"));
    push_ray_with_z(action, 0.2);
    push_ray_with_z(action, -0.2);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

const EditBotAction* edit_woody_skill() {
  static const EditBotAction e = [](Value& action, CondMaker& cond) -> Value {
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));
    Object* o = as_object(action);
    if (o != nullptr) o->set(u"expression", Value(cond.done()));
    return action;
  };
  return &e;
}

}

BotMaker make_bot_data_firen() {
  BotMaker m(oid::kFiren);
  m.set_actions({
      as_action(bot_ball_dfa(num(75), Value(), num(50))),
      as_action(bot_ball_continuation(u"d>a+a", num(0.8), num(75))),
      as_action(bot_ball_dfj(num(75), Value(), num(50), num(1000))(edit_firen_dfj())),
      as_action(bot_ball_dfj(num(0), Value(), num(0), num(1000))(edit_mark_cancel())),
      as_action(bot_ball_dfj(num(75), Value(), num(50), num(200))(edit_firen_dvj())),
      as_action(bot_ball_dfj(num(0), Value(), num(50), num(200))(edit_firen_cancel_dvj())),
      as_action(bot_explosion_duj(num(300), num(1.0 / 60.0), num(-110), num(110), num(900))),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a"), sv(u"d>j"), sv(u"d^j"), sv(u"dvj")}));
  m.set_frames(range_array(255.0, 261.0), arr({sv(u"cancel_d>j")}));
  m.set_frames(range_array(267.0, 275.0), arr({sv(u"cancel_dvj")}));
  m.set_frames(range_array(235.0, 252.0), arr({sv(u"d>a+a")}));
  return m;
}

BotMaker make_bot_data_firzen() {
  BotMaker m(oid::kFirzen);
  m.set_actions({
      as_action(bot_ball_dfj(num(50))),
      as_action(bot_explosion_duj(num(250), Value(), num(-500), num(500), num(500))),
      as_action(bot_explosion_duj(num(250), Value(), num(-250), num(250), num(250))(
          edit_firzen_dj2())),
      as_action(bot_ball_cancelling(u"cancel_d>j")),
      as_action(bot_explosion_dua(num(100), Value(), num(-700), num(700), num(500))),
      as_action(bot_chasing_action(u"d^a+a", arr({sv(u"a")}), Value(),
                                   num(probability(2.0, 0.1)))),
  });
  m.set_frames(arr({sv(u"270"), sv(u"271"), sv(u"272"), sv(u"273")}),
               arr({sv(u"cancel_d>j")}));
  m.set_frames(arr({sv(u"243"), sv(u"244"), sv(u"246")}), arr({sv(u"d^a+a")}));
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings"),
                       frames_field(u"defends")}),
               arr({sv(u"d^j"), sv(u"d^j_2"), sv(u"d^a"), sv(u"d>j")}));
  return m;
}

BotMaker make_bot_data_henry() {
  BotMaker m(oid::kHenry);
  m.set_dataset(obj({{u"w_atk_m_x", num(120)},
                     {u"w_atk_r_x", num(250)},
                     {u"w_atk_x", num(350)}}));
  m.set_actions({
      as_action(bot_ball_dfa(num(150), Value(), num(120), num(800))),
      as_action(bot_ball_dfj(num(200), Value(), num(120), num(1000), num(0.3))),
      as_action(bot_ball_dfj(num(150), Value(), num(120), num(300))(edit_henry_dja(1))),
      as_action(bot_ball_dfj(num(150), Value(), num(300), num(500))(edit_henry_dja(2))),
      as_action(bot_explosion_duj(num(350), Value(), num(-250), num(250), num(90000))),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a"), sv(u"d>j"), sv(u"dja_1"), sv(u"dja_2"), sv(u"d^j")}));
  return m;
}

BotMaker make_bot_data_julian() {
  BotMaker m(oid::kJulian);
  CondMaker c1;
  c1.add(sv(entity_val::kMP), u">", Value(25.0));
  CondMaker c2;
  c2.add(sv(entity_val::kMP), u">", Value(25.0));
  c2.and_(sv(entity_val::kShaking), u">", Value(0.0));
  m.set_actions({
      as_action(bot_chasing_skill_action(u"d>a", Value(), num(25), num(1.0 / 60.0))),
      as_action(bot_chasing_action(u"d>a+a", arr({sv(u"a")}), Value(), num(0.15))),
      as_action(bot_ball_dfj(num(125))),
      as_action(bot_explosion_duj(num(100), num(DESIRE_RATIO_X_3), num(-120), num(120), num(100))),
      as_action(bot_uppercut_dua(num(0))),
      as_action(obj({{u"action_id", sv(u"injured_dja")},
                     {u"desire", Value(defines::desire(0.08))},
                     {u"expression", Value(c1.done())},
                     {u"keys", arr({sv(gk::kd), sv(gk::kj), sv(gk::ka)})}})),
      as_action(obj({{u"action_id", sv(u"shaking_dja")},
                     {u"desire", Value(defines::desire(0.08))},
                     {u"expression", Value(c2.done())},
                     {u"keys", arr({sv(gk::kd), sv(gk::kj), sv(gk::ka)})}})),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings"),
                       frames_field(u"runnings")}),
               arr({sv(u"shaking_dja"), sv(u"d^a"), sv(u"d^j"), sv(u"d>j"), sv(u"d>a")}));
  m.set_frames(concat({frames_field(u"punchs")}), arr({sv(u"d^a"), sv(u"shaking_dja")}));
  m.set_states(arr({num(static_cast<double>(StateEnum::Attacking))}),
               arr({sv(u"shaking_dja"), sv(u"d>a+a")}));
  return m;
}

BotMaker make_bot_data_louisex() {
  BotMaker m(oid::kLouisEX);
  m.set_actions({
      as_action(bot_ball_dfa(num(100), Value(), num(150), num(400))),
      as_action(bot_ball_continuation(u"d>a+a", num(0.5), num(100))),
      as_action(bot_uppercut_dva(num(0), Value(), num(-10), num(120))),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a"), sv(u"dva")}));
  m.set_frames(concat({frames_field(u"punchs")}), arr({sv(u"dva")}));
  m.set_frames(range_array(260.0, 269.0), arr({sv(u"d>a+a")}));
  m.set_dataset(obj({{u"w_atk_x", num(90)},
                     {u"j_atk_x", num(90)},
                     {u"d_atk_max_x", num(200)},
                     {u"r_atk_x", num(200)}}));
  return m;
}

BotMaker make_bot_data_woody() {
  BotMaker m(oid::kWoody);
  m.set_actions({
      as_action(bot_ball_dfa(num(125), Value(), num(50))(edit_woody_dfa())),
      as_action(bot_ball_dfj(num(200), Value(), num(50), num(200))(edit_woody_dfj())),
      as_action(bot_ball_dfj(num(200), num(DESIRE_RATIO_X_4), num(50), num(200))(
          edit_woody_c_dj())),
      as_action(bot_uppercut_dua(num(0), num(DESIRE_RATIO_X_4))),
      as_action(bot_chasing_skill_action(u"d^j", Value(), num(50),
                                         num(DESIRE_RATIO_D_4))(edit_woody_skill())),
      as_action(bot_chasing_skill_action(u"dvj", Value(), num(50), num(DESIRE_RATIO_D_4))),
      as_action(bot_chasing_skill_action(u"d^a", sv(u"catching_d^a"), num(0),
                                         num(DESIRE_RATIO_X_4))),
      as_action(bot_uppercut_dva(num(0), Value(), num(80), num(kUppercutDuaMaxX))),
  });
  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}), arr({sv(u"catching_d^a")}));
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a"), sv(u"d^a"), sv(u"dva"), sv(u"d>j"), sv(u"d^j"), sv(u"dvj")}));
  m.set_states(arr({num(static_cast<double>(StateEnum::Jump))}), arr({sv(u"d^j"), sv(u"dvj")}));
  m.set_frames(concat({frames_field(u"rowings")}), arr({sv(u"dva")}));
  m.set_frames(arr({num(286), num(287), num(288), num(301), num(302), num(303), num(271)}),
               arr({sv(u"c_d>j"), sv(u"d^a")}));
  m.set_frames(arr({num(215), num(219)}), arr({sv(u"c_d>j"), sv(u"d^a")}));
  return m;
}

void register_all_bots() {
  BotMaker::register_maker(oid::kBat, make_bot_data_bat);
  BotMaker::register_maker(oid::kDavis, make_bot_data_davis);
  BotMaker::register_maker(oid::kFiren, make_bot_data_firen);
  BotMaker::register_maker(oid::kFirzen, make_bot_data_firzen);
  BotMaker::register_maker(oid::kHenry, make_bot_data_henry);
  BotMaker::register_maker(oid::kHunter, make_bot_data_hunter);
  BotMaker::register_maker(oid::kJack, make_bot_data_jack);
  BotMaker::register_maker(oid::kJan, make_bot_data_jan);
  BotMaker::register_maker(oid::kJulian, make_bot_data_julian);
  BotMaker::register_maker(oid::kJustin, make_bot_data_justin);
  BotMaker::register_maker(oid::kKnight, make_bot_data_knight);
  BotMaker::register_maker(oid::kLouis, make_bot_data_louis);
  BotMaker::register_maker(oid::kLouisEX, make_bot_data_louisex);
  BotMaker::register_maker(oid::kMark, make_bot_data_mark);
  BotMaker::register_maker(oid::kMonk, make_bot_data_monk);
  BotMaker::register_maker(oid::kSorcerer, make_bot_data_sorcerer);
  BotMaker::register_maker(oid::kWoody, make_bot_data_woody);
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

BotMaker make_bot_data_davis() {
  BotMaker m(oid::kDavis);
  m.set_actions({
      as_action(obj({{u"action_id", sv(u"dont_stop_run_attack")},
                     {u"keys", arr(std::vector<Value>())},
                     {u"desire", Value(defines::num(u"Defines.MAX_AI_DESIRE"))}})),
      as_action(bot_ball_dfa(num(50), Value(), num(50))),
      as_action(bot_ball_continuation(u"d>a+a", num(probability(3.0, 0.8)), num(50))),
      as_action(bot_uppercut_dua(num(225), num(DESIRE_RATIO_X_4), num(kUppercutDuaMinX),
                                num(kUppercutDuaMaxX))),
      as_action(bot_uppercut_dva(num(75), num(DESIRE_RATIO_X_4), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))),
      as_action(bot_uppercut_dva(num(25), num(probability(5.0, 0.5)), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))(edit_davis_dva_j())),
      as_action(bot_uppercut_dva(num(25), Value(), num(kUppercutDvaMaxX),
                                 num(kUppercutDvaMinX + kUppercutDvaMaxX))(edit_davis_dj())),
      as_action(bot_uppercut_dva(num(0), num(1.0), num(kUppercutDvaMinX), num(40))(edit_davis_dj_a())),
      as_action(bot_chasing_action(u"dva+run", arr({sv(u"F"), sv(u"F")}), num(0),
                                   num(probability(7.0, 0.5)))),
  });
  m.set_states(arr({num(static_cast<double>(StateEnum::Rowing))}),
               arr({sv(u"d^a"), sv(u"d^j")}));
  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}),
               arr({sv(u"d^a"), sv(u"dva")}));
  m.set_frames(arr({sv(u"87")}), arr({sv(u"dont_stop_run_attack")}));
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings"),
                       frames_field(u"runnings")}),
               arr({sv(u"d^a"), sv(u"d^j"), sv(u"d>a"), sv(u"dva")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"dva"), sv(u"d^a")}));
  m.set_frames(range_array(240.0, 269.0), arr({sv(u"d>a+a")}));
  m.set_frames(arr({num(282)}), arr({sv(u"dva+run")}));
  m.set_frames(range_array(270.0, 289.0), arr({sv(u"dva+j"), sv(u"d^a")}));
  m.set_frames(arr({num(39)}), arr({sv(u"dva+j"), sv(u"d^a")}));
  m.set_frames(range_array(290.0, 292.0), arr({sv(u"d^j+a")}));
  return m;
}

BotMaker make_bot_data_jack() {
  BotMaker m(oid::kJack);
  m.set_actions({
      as_action(bot_ball_dfa(num(40), Value(), num(50))),
      as_action(bot_ball_continuation(u"d>a+d>a", num(probability(3.0, 0.3)), num(40),
                                      arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}))),
      as_action(bot_uppercut_dua(num(225), num(1.0 / 15.0), num(kUppercutDuaMinX),
                                 num(kUppercutDuaMaxX))),
  });
  m.set_states(arr({num(static_cast<double>(StateEnum::Rowing)),
                    num(static_cast<double>(StateEnum::Catching))}),
               arr({sv(u"d^a")}));
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^a"), sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d^a")}));
  m.set_frames(range_array(240.0, 247.0), arr({sv(u"d>a+d>a")}));
  return m;
}

BotMaker make_bot_data_justin() {
  BotMaker m(oid::kJustin);
  m.set_actions({
      as_action(bot_ball_dfa(num(75), Value(), num(50), num(200))),
      as_action(bot_front_test(u"dva", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)}), num(75),
                               Value(), num(-10), num(100))),
      as_action(bot_chasing_action(u"d>a+a", arr({sv(gk::ka)}), num(75), Value())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"dva"), sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"dva")}));
  m.set_frames(range_array(240.0, 246.0), arr({sv(u"d>a+a")}));
  return m;
}

BotMaker make_bot_data_louis() {
  BotMaker m(oid::kLouis);
  m.set_actions({
      as_action(bot_ball_dfa(num(150), Value(), num(120), num(800))),
      as_action(bot_ball_dfj(num(50), Value(), num(120), num(250))),
      as_action(bot_uppercut_duj(num(100), Value(), num(-10), num(120))),
      as_action(bot_chasing_skill_action(u"dja", Value(), Value(), num(0.01))(edit_louis_dja())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^j"), sv(u"d>a"), sv(u"d>j"), sv(u"dja")}));
  m.set_frames(concat({frames_field(u"punchs")}), arr({sv(u"d^j")}));
  return m;
}

BotMaker make_bot_data_mark() {
  BotMaker m(oid::kMark);
  m.set_actions({
      as_action(bot_ball_dfa(num(0), Value(), num(0), num(80), num(0.1))(edit_mark_rays_max_d())),
      as_action(bot_chasing_action(u"d>a+a", arr({sv(gk::ka)}), num(0), num(1.0 / 15.0))),
      as_action(bot_ball_dfj(num(50), Value(), num(20), num(200), num(0.1))(edit_mark_dfj())),
      as_action(bot_ball_dfj(num(0), Value(), num(0), num(1000))(edit_mark_cancel())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>j"), sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>j"), sv(u"d>a")}));
  m.set_frames(range_array(240.0, 244.0), arr({sv(u"cancel_d>j")}));
  m.set_frames(range_array(85.0, 89.0), arr({sv(u"d>a+a")}));
  return m;
}

BotMaker make_bot_data_sorcerer() {
  BotMaker m(oid::kSorcerer);
  m.set_actions({
      as_action(bot_ball_dfa(num(75), Value(), num(100), num(10000))),
      as_action(bot_ball_dfj(num(125), Value(), num(100), num(10000))),
      as_action(bot_idle_action(u"dvj", arr({sv(gk::kDefend), sv(gk::kDown), sv(gk::kJump)}),
                                num(350))(edit_sorcerer_status())),
      as_action(bot_idle_action(u"d^j", arr({sv(gk::kDefend), sv(gk::kUp), sv(gk::kJump)}),
                                num(350))(edit_sorcerer_status())),
  });
  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a"), sv(u"d>j"), sv(u"d^j"), sv(u"dvj")}));
  return m;
}

}
}
}
