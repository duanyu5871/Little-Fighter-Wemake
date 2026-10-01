#include "lfw/dat_translator/frame_behavior.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/chase_lost.h"
#include "lfw/defines/chase_strategy.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/hit_flag.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/opoint_kind.h"
#include "lfw/defines/opoint_multi_enum.h"
#include "lfw/defines/opoint_spreading.h"
#include "lfw/defines/speed_ctrl.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/foreach.h"

namespace lfw {
namespace dat_translator {
namespace {

std::u16string hp_gt_0_text() {
  CondMaker cm;
  cm.and_(s(entity_val::kHP), u">", n(0));
  return cm.done();
}

std::u16string victim_chasing_text() {
  CondMaker cm;
  cm.and_(s(collision_val::kVictimIsChasing), u"==", n(1));
  return cm.done();
}

std::pair<Value, Value> hit_flag_pair(const Value& value) {
  Object tmp;
  set_hit_flag(tmp, value);
  const Value* flag = tmp.get(u"hit_flag");
  const Value* name = tmp.get(u"hit_flag_name");
  return {flag != nullptr ? *flag : Value(), name != nullptr ? *name : Value()};
}

double sub(const Value& v, double d) { return to_number(v) - d; }

double add(const Value& v, double d) { return to_number(v) + d; }

void set_default_speed(Object& o, double dvx, double acc_x, double acc_y, double acc_z) {
  o.set(u"facing", en(FacingFlag::VX));
  o.set(u"dvx", n(dvx));
  o.set(u"acc_x", n(acc_x));
  o.set(u"vxm", en(SpeedMode::AccTo));
  o.set(u"dvz", n(defines::num(u"Defines.DEFAULT_OPOINT_SPEED_Z")));
  o.set(u"acc_z", n(acc_z));
  o.set(u"vzm", en(SpeedMode::AccTo));
  o.set(u"dvy", n(-0.5));
  o.set(u"acc_y", n(acc_y));
  o.set(u"vym", en(SpeedMode::AccTo));
  o.set(u"ctrl_z", n(1));
  o.set(u"ctrl_y", n(1));
  o.set(u"ctrl_x", n(1));
}

void put_opoint(Value& frame, Object& o, std::vector<Value> items) {
  Value existing = field_or(frame, u"opoint");
  o.set(u"opoint", ensure(existing, items));
}

}

void make_fb_bat_chase_start(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", s(oid::kBatChase)},
      {u"x", field_or(frame, u"centerx")},
      {u"y", field_or(frame, u"centery")},
      {u"action", make_obj({{u"id", s(u"0")}})},
      {u"multi", make_obj({{u"type", en(OpointMultiEnum::AccordingEnemies)},
                           {u"min", n(3)},
                           {u"skip_zero", Value(false)}})},
      {u"spreading", en(OpointSpreading::Spreading)},
      {u"gen_spread_x", s(u"bag(-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6)")},
      {u"gen_spread_z", s(u"bag(-2,-1,0,1,2)")}}));
  put_opoint(frame, *p, items);
}

void make_fb_bat_chase(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  set_default_speed(o, 14, 0.25, 0.125, 0.125);
  o.set(u"chase", make_obj({{u"flag", en(HitFlag::EnemyFighter)},
                            {u"stratedy", en(ChaseStrategy::UntilLost)},
                            {u"lost", en(ChaseLost::Hover)}}));
}

void make_fb_boomerang(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  o.set(u"dvx", n(20));
  o.set(u"acc_x", n(0.2));
  o.set(u"vxm", en(SpeedMode::AccTo));
  o.set(u"ctrl_x", en(SpeedCtrl::Control));
  o.set(u"dvz", n(1.8));
  o.set(u"acc_z", n(0.1));
  o.set(u"vzm", en(SpeedMode::AccTo));
  o.set(u"ctrl_z", en(SpeedCtrl::Control));
  o.set(u"chase", make_obj({{u"flag", en(HitFlag::EnemyFighter)},
                            {u"lost", en(ChaseLost::Leave)}}));
}

void make_fb_chasing_same_enemy(Value& frame, const std::u16string& oid) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  o.set(u"facing", en(FacingFlag::VX));
  o.set(u"dvx", n(14));
  o.set(u"acc_x", n(0.25));
  o.set(u"vxm", en(SpeedMode::AccTo));
  o.set(u"dvz", n(defines::num(u"Defines.DEFAULT_OPOINT_SPEED_Z")));
  o.set(u"acc_z", n(0.25));
  o.set(u"vzm", en(SpeedMode::AccTo));
  o.set(u"dvy", n(8));
  o.set(u"acc_y", n(-0.25));
  o.set(u"vym", en(SpeedMode::AccTo));
  o.set(u"ctrl_z", n(1));
  o.set(u"ctrl_x", n(1));
  const bool firzen = oid == std::u16string(oid::kFirzenChasef) ||
                      oid == std::u16string(oid::kFirzenChasei);
  Value itrs = field_or(frame, u"itr");
  foreach (itrs, [&firzen](Value& item, size_t) {
    Object* it = as_object(item);
    if (it == nullptr) return;
    it->set(u"on_hit_ground", make_obj({{u"id", s(firzen ? u"60" : u"10")}}));
  });
  o.set(u"chase", make_obj({{u"stratedy", en(ChaseStrategy::UntilLost)},
                            {u"flag", en(HitFlag::EnemyFighter)},
                            {u"lost", en(ChaseLost::Hover)}}));
}

void make_fb_dennis_chase(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  set_default_speed(o, 14, 0.25, 0.25, 0.25);
  o.set(u"on_dead", make_obj({{u"id", s(u"5")}}));
  const Value id = field_or(frame, u"id");
  const std::u16string expr = hp_gt_0_text();
  if (same_str(id, u"1")) {
    o.set(u"key_down", make_obj({{u"F", make_obj({{u"id", s(u"3")},
                                                  {u"wait", s(u"i")},
                                                  {u"expression", Value(expr)}})}}));
  } else if (same_str(id, u"2")) {
    o.set(u"key_down", make_obj({{u"F", make_obj({{u"id", s(u"4")},
                                                  {u"wait", s(u"i")},
                                                  {u"expression", Value(expr)}})}}));
  } else if (same_str(id, u"3")) {
    o.set(u"key_down", make_obj({{u"B", make_obj({{u"id", s(u"1")},
                                                  {u"wait", s(u"i")},
                                                  {u"expression", Value(expr)}})}}));
  } else if (same_str(id, u"4")) {
    o.set(u"key_down", make_obj({{u"B", make_obj({{u"id", s(u"2")},
                                                  {u"wait", s(u"i")},
                                                  {u"expression", Value(expr)}})}}));
  }
  o.set(u"chase", make_obj({{u"flag", en(HitFlag::EnemyFighter)},
                            {u"lost", en(ChaseLost::Hover)},
                            {u"oy", n(0.5)}}));
}

void make_fb_firzen_disater_start(Value& frame, std::optional<Value> x, std::optional<Value> y) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  const Value xv = x.has_value() ? *x : field_or(frame, u"centerx");
  const Value yv = y.has_value() ? *y : field_or(frame, u"centery");
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", make_arr({s(oid::kFirzenChasef), s(oid::kFirzenChasei)})},
      {u"x", xv},
      {u"y", yv},
      {u"dvy", n(6)},
      {u"action", make_obj({{u"id", s(u"0")}})},
      {u"multi", make_obj({{u"type", en(OpointMultiEnum::AccordingEnemies)},
                           {u"min", n(4)},
                           {u"skip_zero", Value(true)}})},
      {u"spreading", en(OpointSpreading::Spreading)},
      {u"gen_spread_x", s(u"bag(-5,-4,-3,-2,-1,0,1,2,3,4,5)")},
      {u"gen_spread_y", s(u"bag(2,2.5,3,3.5,4,4.5,5,5.5,6,6.5,7,7.5,8)")}}));
  put_opoint(frame, *p, items);
}

void make_fb_firzen_volcano_start(Value& frame, std::optional<Value> x, std::optional<Value> y) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  make_fb_firzen_disater_start(frame, x, y);
  const Value cx = field_or(frame, u"centerx");
  std::vector<Value> items;
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", cx},
                            {u"y", n(24)},
                            {u"action", make_obj({{u"id", s(u"109")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(135)},
                            {u"y", n(24)},
                            {u"action", make_obj({{u"id", s(u"100")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(135)},
                            {u"y", n(24)},
                            {u"z", n(-60)},
                            {u"dvz", n(-4)},
                            {u"action", make_obj({{u"id", s(u"100")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(135)},
                            {u"y", n(24)},
                            {u"z", n(60)},
                            {u"dvz", n(4)},
                            {u"action", make_obj({{u"id", s(u"100")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(-45)},
                            {u"y", n(24)},
                            {u"action", make_obj({{u"id", s(u"100")},
                                                   {u"facing", en(FacingFlag::Backward)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(-45)},
                            {u"y", n(24)},
                            {u"z", n(-60)},
                            {u"dvz", n(-4)},
                            {u"action", make_obj({{u"id", s(u"100")},
                                                   {u"facing", en(FacingFlag::Backward)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(-45)},
                            {u"y", n(24)},
                            {u"z", n(60)},
                            {u"dvz", n(4)},
                            {u"action", make_obj({{u"id", s(u"100")},
                                                   {u"facing", en(FacingFlag::Backward)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", cx},
                            {u"y", n(26)},
                            {u"z", n(0)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 25))},
                            {u"y", n(26)},
                            {u"z", n(0)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 25))},
                            {u"y", n(26)},
                            {u"z", n(0)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 50))},
                            {u"y", n(26)},
                            {u"z", n(0)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 50))},
                            {u"y", n(26)},
                            {u"z", n(0)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 38))},
                            {u"y", n(26)},
                            {u"z", n(-15)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 38))},
                            {u"y", n(26)},
                            {u"z", n(-15)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 38))},
                            {u"y", n(26)},
                            {u"z", n(15)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 38))},
                            {u"y", n(26)},
                            {u"z", n(15)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 10))},
                            {u"y", n(26)},
                            {u"z", n(25)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 10))},
                            {u"y", n(26)},
                            {u"z", n(25)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(add(cx, 10))},
                            {u"y", n(26)},
                            {u"z", n(-25)},
                            {u"action", make_obj({{u"id", s(u"54")}})}}));
  items.push_back(make_obj({{u"kind", en(OpointKind::Normal)},
                            {u"oid", s(oid::kFirenFlame)},
                            {u"x", n(sub(cx, 10))},
                            {u"y", n(26)},
                            {u"z", n(-25)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));
  put_opoint(frame, *p, items);
}

void make_fb_jan_angle_blessing(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  set_default_speed(o, 14, 0.25, 0.25, 0.25);
  const std::pair<Value, Value> hf = hit_flag_pair(en(HitFlag::AllyFighter));
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(ItrKind::Heal)},
      {u"x", n(25)},
      {u"y", n(13)},
      {u"w", n(32)},
      {u"h", n(34)},
      {u"injury", n(100)},
      {u"hit_flag", hf.first},
      {u"hit_flag_name", hf.second},
      {u"actions", make_arr({make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                                       {u"data", make_obj({{u"id", s(u"60")}})}})})},
      {u"test", Value(victim_chasing_text())}}));
  Value existing = field_or(frame, u"itr");
  o.set(u"itr", ensure(existing, items));
  o.set(u"chase", make_obj({{u"stratedy", en(ChaseStrategy::StopOnLost)},
                            {u"flag", en(HitFlag::AllyFighter)},
                            {u"lost", en(ChaseLost::Leave)}}));
}

void make_fb_jan_chase_start(Value& frame, std::optional<Value> x, std::optional<Value> y) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  const Value xv = x.has_value() ? *x : field_or(frame, u"centerx");
  const Value yv = y.has_value() ? *y : field_or(frame, u"centery");
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", s(oid::kJanChase)},
      {u"x", xv},
      {u"y", yv},
      {u"dvy", n(6)},
      {u"action", make_obj({{u"id", s(u"0")}})},
      {u"multi", make_obj({{u"type", en(OpointMultiEnum::AccordingEnemies)},
                           {u"min", n(1)},
                           {u"skip_zero", Value(true)}})},
      {u"spreading", en(OpointSpreading::Spreading)},
      {u"gen_spread_x", s(u"bag(-5,-4,-3,-2,-1,0,1,2,3,4,5)")},
      {u"gen_spread_y", s(u"bag(2,2.5,3,3.5,4,4.5,5,5.5,6,6.5,7,7.5,8)")}}));
  put_opoint(frame, *p, items);
}

void make_fb_jan_chaseh_start(Value& frame, std::optional<Value> x, std::optional<Value> y) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  const Value xv = x.has_value() ? *x : field_or(frame, u"centerx");
  const Value yv = y.has_value() ? *y : field_or(frame, u"centery");
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", s(oid::kJanChaseh)},
      {u"x", xv},
      {u"y", yv},
      {u"action", make_obj({{u"id", s(u"0")}})},
      {u"multi", make_obj({{u"type", en(OpointMultiEnum::AccordingAllies)}})},
      {u"spreading", en(OpointSpreading::Spreading)}}));
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", s(oid::kJanChaseh)},
      {u"x", xv},
      {u"y", n(add(yv, 40))},
      {u"action", make_obj({{u"id", s(u"0")}})},
      {u"multi", make_obj({{u"type", en(OpointMultiEnum::Emitter)}})}}));
  put_opoint(frame, *p, items);
}

void make_fb_john_chase_leaving(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  o.set(u"facing", en(FacingFlag::VX));
  o.set(u"dvx", n(30));
  o.set(u"acc_x", n(2));
  o.set(u"vxm", en(SpeedMode::AccTo));
}

void make_fb_john_chase(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  set_default_speed(o, 13, 0.25, 0.25, 0.25);
  o.set(u"chase", make_obj({{u"flag", en(HitFlag::EnemyFighter)},
                            {u"lost", en(ChaseLost::Hover)},
                            {u"oy", n(0)}}));
}

void make_fb_julian_ball_start(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  std::vector<Value> items;
  items.push_back(make_obj({
      {u"kind", en(OpointKind::Normal)},
      {u"oid", s(oid::kJulianBall)},
      {u"x", field_or(frame, u"centerx")},
      {u"y", field_or(frame, u"centery")},
      {u"dvx", n(8)},
      {u"action", make_obj({{u"id", s(u"50")}})},
      {u"spreading", en(OpointSpreading::FloatRange)},
      {u"gen_spread_z", s(u"rand(-15, 15) / 10")},
      {u"gen_spread_y", s(u"rand(-5, 5) / 10")}}));
  put_opoint(frame, *p, items);
}

void make_fb_julian_ball(Value& frame) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  Object& o = *p;
  o.set(u"facing", en(FacingFlag::VX));
  o.set(u"dvx", n(12));
  o.set(u"acc_x", n(0.18));
  o.set(u"vxm", en(SpeedMode::AccTo));
  o.set(u"dvz", n(defines::num(u"Defines.DEFAULT_OPOINT_SPEED_Z")));
  o.set(u"acc_z", n(0.05));
  o.set(u"vzm", en(SpeedMode::AccTo));
  o.set(u"dvy", n(-0.5));
  o.set(u"acc_y", n(0.1));
  o.set(u"vym", en(SpeedMode::AccTo));
  o.set(u"ctrl_z", n(1));
  o.set(u"ctrl_y", n(1));
  o.set(u"ctrl_x", n(1));
  const double fid = to_number(field_or(frame, u"id"));
  o.set(u"chase", make_obj({{u"flag", en(HitFlag::EnemyFighter)},
                            {u"lost", en(ChaseLost::Hover)},
                            {u"overshoot", make_obj({{u"x", n(80)},
                                                     {u"y", n(80)},
                                                     {u"z", n(80)}})}}));
  const std::u16string expr = hp_gt_0_text();
  if (fid >= 50 && fid <= 59) {
    o.set(u"key_down",
          make_obj({{u"F", make_obj({{u"id", Value(number_to_string(fid + 50))},
                                     {u"wait", s(u"i")},
                                     {u"expression", Value(expr)}})}}));
  } else if (fid >= 1 && fid <= 9) {
    o.set(u"key_down",
          make_obj({{u"B", make_obj({{u"id", Value(number_to_string(fid + 50))},
                                     {u"wait", s(u"i")},
                                     {u"expression", Value(expr)}})}}));
  }
}

void make_frame_behavior(Value& frame, const std::u16string& oid) {
  Object* p = frame_obj(frame);
  if (p == nullptr) return;
  const Value behavior = field_or(frame, u"behavior");
  if (strict_equals(behavior, en(FrameBehavior::AngelBlessing))) {
    make_fb_jan_angle_blessing(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::JohnChase))) {
    make_fb_john_chase(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::DennisChase))) {
    make_fb_dennis_chase(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::Boomerang))) {
    make_fb_boomerang(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::AngelBlessingStart))) {
    make_fb_jan_chaseh_start(frame, std::nullopt, std::nullopt);
  } else if (strict_equals(behavior, en(FrameBehavior::DevilJudgementStart))) {
    make_fb_jan_chase_start(frame, std::nullopt, std::nullopt);
  } else if (strict_equals(behavior, en(FrameBehavior::ChasingSameEnemy))) {
    make_fb_chasing_same_enemy(frame, oid);
  } else if (strict_equals(behavior, en(FrameBehavior::BatStart))) {
    make_fb_bat_chase_start(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::FirzenDisasterStart))) {
    make_fb_firzen_disater_start(frame, std::nullopt, std::nullopt);
  } else if (strict_equals(behavior, en(FrameBehavior::JohnBiscuitLeaving))) {
    make_fb_john_chase_leaving(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::FirzenVolcanoStart))) {
    make_fb_firzen_volcano_start(frame, field_or(frame, u"centerx"), n(-79));
  } else if (strict_equals(behavior, en(FrameBehavior::Bat))) {
    make_fb_bat_chase(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::JulianBallStart))) {
    make_fb_julian_ball_start(frame);
  } else if (strict_equals(behavior, en(FrameBehavior::JulianBall))) {
    make_fb_julian_ball(frame);
  }
}

}

}
