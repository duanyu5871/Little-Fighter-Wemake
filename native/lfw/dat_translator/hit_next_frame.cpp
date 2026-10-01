#include "lfw/dat_translator/hit_next_frame.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/assign.h"

namespace lfw {
namespace dat_translator {

namespace {

double wt(WeaponEnum v) { return static_cast<double>(v); }

}

Value hit_next_frame_drink() {
  CondMaker cond;
  cond.one_of(s(entity_val::kHolding_W_Type), {n(wt(WeaponEnum::Drink))});
  return make_arr({make_obj({{u"id", s(u"55")},
                             {u"desc", s(u"drink")},
                             {u"mp_mode", n(1)},
                             {u"expression", Value(cond.done())}})});
}

Value hit_next_frame_super_punch() {
  CondMaker cond;
  cond.add(s(entity_val::kRequireSuperPunch), u">", n(0));
  return make_arr({make_obj({{u"id", s(u"70")},
                             {u"mp_mode", n(1)},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"desc", s(u"super_punch")},
                             {u"expression", Value(cond.done())}})});
}

Value hit_next_frame_punch() {
  return make_arr({make_obj({{u"mp_mode", n(1)},
                             {u"id", make_arr({s(u"60"), s(u"65")})},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"desc", s(u"punch")}})});
}

void hit_next_frame_turn_back(Value& frame, const Value& back_frame) {
  Object* fo = as_object(frame);
  if (fo == nullptr) return;
  if (equals(back_frame, Value())) {
    fo->set(u"facing", en(FacingFlag::Ctrl));
    return;
  }
  const Value inner = make_obj({{u"id", back_frame},
                                {u"wait", s(u"i")},
                                {u"facing", en(FacingFlag::Backward)}});
  const Value back = make_obj({{u"B", inner}});
  const Value* kd = fo->get(u"key_down");
  const Value key_down = kd != nullptr ? *kd : Value();
  fo->set(u"key_down", assign(key_down, back));
  const Value* hit = fo->get(u"hit");
  const Value hit_v = hit != nullptr ? *hit : Value();
  fo->set(u"hit", assign(hit_v, back));
}

Value hit_next_frame_jump() {
  return make_arr({make_obj({{u"id", s(u"210")},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"mp_mode", n(1)}})});
}

Value hit_next_frame_defend() {
  return make_arr({make_obj({{u"id", s(u"110")},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"mp_mode", n(1)}})});
}

Value hit_next_frame_weapon_atk() {
  CondMaker c1;
  c1.add(s(entity_val::kHolding_W_Type), u"==", n(wt(WeaponEnum::Baseball)));
  c1.or_([&](CondMaker& cc) {
    cc.add(s(entity_val::kHolding_W_Type), u"==", n(wt(WeaponEnum::Knife)));
    cc.and_(s(entity_val::kPressFB), u"!=", n(0));
    return &cc;
  });
  CondMaker c2;
  c2.one_of(s(entity_val::kHolding_W_Type),
            {n(wt(WeaponEnum::Knife)), n(wt(WeaponEnum::Stick))});
  return make_arr({make_obj({{u"mp_mode", n(1)},
                             {u"id", s(u"45")},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"expression", Value(c1.done())}}),
                   make_obj({{u"mp_mode", n(1)},
                             {u"id", make_arr({s(u"20"), s(u"25")})},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"expression", Value(c2.done())}})});
}

Value hit_next_frame_jump_atk() {
  CondMaker c1;
  c1.one_of(s(entity_val::kHolding_W_Type),
            {n(wt(WeaponEnum::Baseball)), n(wt(WeaponEnum::Drink))});
  c1.or_([&](CondMaker& cc) {
    cc.add(s(entity_val::kPressFB), u"!=", n(0));
    cc.and_(s(entity_val::kHolding_W_Type), u"!=", n(wt(WeaponEnum::None)));
    return &cc;
  });
  CondMaker c2;
  c2.one_of(s(entity_val::kHolding_W_Type),
            {n(wt(WeaponEnum::Knife)), n(wt(WeaponEnum::Stick))});
  return make_arr({make_obj({{u"id", s(u"52")},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"desc", s(u"\u7a7a\u4e2d\u4e22\u51fa\u6b66\u5668")},
                             {u"mp_mode", n(1)},
                             {u"expression", Value(c1.done())}}),
                   make_obj({{u"id", s(u"30")},
                             {u"facing", en(FacingFlag::Ctrl)},
                             {u"desc", s(u"\u7a7a\u4e2d\u6b66\u5668\u653b\u51fb")},
                             {u"mp_mode", n(1)},
                             {u"expression", Value(c2.done())}}),
                   make_obj({{u"id", s(u"80")},
                             {u"desc", s(u"\u8df3\u8dc3\u653b\u51fb")},
                             {u"facing", en(FacingFlag::Ctrl)}})});
}

}
}
