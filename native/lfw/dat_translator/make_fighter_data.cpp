#include "lfw/dat_translator/make_fighter_data.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/bots_frames.h"
#include "lfw/dat_translator/cond_maker.h"
#include "lfw/dat_translator/cook_file_variants.h"
#include "lfw/dat_translator/frame_editing.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/hit_next_frame.h"
#include "lfw/dat_translator/make_frame_state.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/dat_translator/value_builder.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/ensure.h"
#include "lfw/utils/container_help/traversal.h"
#include "lfw/utils/type_cast.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {

namespace {

Value ev(const char16_t* key) { return Value(std::u16string(key)); }

Value wt(WeaponEnum v) { return Value(static_cast<double>(v)); }

Value ff(FacingFlag v) { return Value(static_cast<double>(v)); }

Value se(StateEnum v) { return Value(static_cast<double>(v)); }

CondMaker& cadd(CondMaker& cm, const Value& a, const char16_t* op, const Value& b) {
  return cm.add(a, std::u16string(op), b);
}

CondMaker& cand(CondMaker& cm, const Value& a, const char16_t* op, const Value& b) {
  return cm.and_(a, std::u16string(op), b);
}

CondMaker& cor(CondMaker& cm, const Value& a, const char16_t* op, const Value& b) {
  return cm.or_(a, std::u16string(op), b);
}

Object* as_mut(const Value& v) { return const_cast<Object*>(as_object(v)); }

Array* arr_mut(const Value& v) { return const_cast<Array*>(as_array(v)); }

const Value* key_of(const Object* o, const char16_t* key) {
  return o != nullptr ? o->get(std::u16string(key)) : nullptr;
}

Value field_at(const Value& v, const char16_t* key) {
  const Value* p = key_of(as_object(v), key);
  return p != nullptr ? *p : Value();
}

void spread(std::vector<Value>& out, const Value& v) {
  if (const Array* a = as_array(v)) {
    for (size_t i = 0; i < a->size(); ++i) out.push_back(a->at(i));
    return;
  }
  out.push_back(v);
}

std::vector<Value> spread_of(const Value& v) {
  std::vector<Value> out;
  spread(out, v);
  return out;
}

bool is_state(const Value& v, StateEnum s) {
  return to_number(v) == static_cast<double>(s);
}

void add_key_down_jump_atk(Value& frame_v) {
  Object* frame = as_mut(frame_v);
  if (frame == nullptr) return;
  const Value* kd = frame->get(u"key_down");
  if (kd == nullptr || !truthy(*kd)) frame->set(u"key_down", Value(std::make_shared<Object>()));
  Object* kdo = as_mut(*frame->get(u"key_down"));
  if (kdo == nullptr) return;
  kdo->set(u"a", add_next_frame(field_at(Value(std::make_shared<Object>(*kdo)), u"a"),
                                spread_of(hit_next_frame_jump_atk())));
}

void cook_marker(const Value& any_next, Object& tframes) {
  std::vector<Value> list;
  if (const Array* a = as_array(any_next)) {
    for (size_t i = 0; i < a->size(); ++i) list.push_back(a->at(i));
  } else {
    if (!truthy(any_next)) return;
    list.push_back(any_next);
  }
  for (const Value& i : list) {
    Object* io = as_mut(i);
    if (io == nullptr) continue;
    const Value* idv = io->get(u"id");    if (idv == nullptr || !truthy(*idv)) continue;
    if (as_array(*idv) != nullptr) continue;
    if (tframes.get(to_string(*idv)) == nullptr) continue;
    CondMaker cm;
    cadd(cm, ev(entity_val::kHAS_TRANSFORM_DATA), u"==", n(1));
    io->set(u"expression", Value(cm.done()));
  }
}

}

static void cook_transform_begin_expression_to_hit(Value& frames_v);

Value make_fighter_data(Value& ctx) {
  Object* co = as_object(ctx);
  if (co == nullptr) return Value();
  const Value* base_v = co->get(u"base");
  Value* frames_v = const_cast<Value*>(co->get(u"frames"));
  const Value* index_v = co->get(u"index");
  if (base_v == nullptr || frames_v == nullptr || index_v == nullptr) return Value();
  if (as_object(*base_v) == nullptr || as_object(*frames_v) == nullptr) return Value();
  Object* base = as_mut(*base_v);
  Object* frames = as_mut(*frames_v);
  const Value dat_index = *index_v;

  const double walking_frame_rate = to_number(take_number(*base, u"walking_frame_rate", n(3)));
  const double running_frame_rate = to_number(take_number(*base, u"running_frame_rate", n(3)));
  const double walking_speed = to_number(take_number(*base, u"walking_speed", n(0)));
  const double walking_speedz = to_number(take_number(*base, u"walking_speedz", n(0)));
  const double running_speed = to_number(take_number(*base, u"running_speed", n(0)));
  const double running_speedz = to_number(take_number(*base, u"running_speedz", n(0)));
  const double heavy_walking_speed = to_number(take_number(*base, u"heavy_walking_speed", n(0)));
  const double heavy_walking_speedz = to_number(take_number(*base, u"heavy_walking_speedz", n(0)));
  const double heavy_running_speed = to_number(take_number(*base, u"heavy_running_speed", n(0)));
  const double heavy_running_speedz = to_number(take_number(*base, u"heavy_running_speedz", n(0)));

  Object round_trip_frames_map;
  Object frame_mp_hp_map;

  traversal(*frames_v, [&frame_mp_hp_map](const std::u16string& frame_id, Value& frame) {
    Object* fo = as_mut(frame);
    if (fo == nullptr) return;
    const std::pair<double, double> v = take_raw_frame_mp(*fo);
    frame_mp_hp_map.set(frame_id, make_obj({{u"mp", n(v.first)}, {u"hp", n(v.second)}}));
  });

  const std::vector<std::u16string> frame_keys = frames->keys();
  for (const std::u16string& frame_id : frame_keys) {
    const Value* fp = frames->get(frame_id);
    if (fp == nullptr) continue;
    Value frame_v = *fp;
    Object* frame = as_mut(frame_v);
    if (frame == nullptr) continue;
    const Value frame_id_val = field_at(frame_v, u"id");

    const Value next_v = field_at(frame_v, u"next");
    if (const Array* na = as_array(next_v)) {
      for (size_t i = 0; i < na->size(); ++i) {
        Value item = na->at(i);
        cook_next_frame_cost(item, u"next", &frame_mp_hp_map);
      }
    } else {
      Value item = next_v;
      cook_next_frame_cost(item, u"next", &frame_mp_hp_map);
    }

    FrameEditing editing(frame_v, &frame_mp_hp_map);
    const Value hit_a = take(*frame, u"hit_a");
    const Value hit_j = take(*frame, u"hit_j");
    const Value hit_d = take(*frame, u"hit_d");

    if (truthy(hit_a)) editing.hit(s(u"a"), {hit_a});
    if (truthy(hit_j)) editing.hit(s(u"j"), {hit_j});
    if (truthy(hit_d)) editing.hit(s(u"d"), {hit_d});

    const char16_t* k9[7] = {u"Fa", u"Fj", u"Da", u"Dj", u"Ua", u"Uj", u"ja"};
    for (const char16_t* k : k9) {
      const Value nxt = take(*frame, std::u16string(u"hit_") + k);
      if (!is_str(nxt) && !is_num(nxt)) continue;
      if (strict_equals(nxt, s(u"0")) || strict_equals(nxt, n(0))) continue;
      editing.seq(k, {nxt});
    }

    const double fid = to_number(frame_id_val);
    const double state = to_number(field_at(frame_v, u"state"));

    if (fid == 0 || fid == 1 || fid == 2 || fid == 3 || fid == 4) {
    } else if (fid == 5 || fid == 6 || fid == 7 || fid == 8) {
      std::vector<Value> nexts;
      nexts.push_back(make_obj({{u"id", s(u"45")},
                                {u"facing", ff(FacingFlag::Ctrl)},
                                {u"desc", s(u"weapon throw")},
                                {u"mp_mode", n(1)},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cadd(cm, ev(entity_val::kHolding_W_Type), u"==",
                                          wt(WeaponEnum::Baseball));
                                   cm.or_([&](CondMaker& c) -> CondMaker* {
                                     cadd(c, ev(entity_val::kHolding_W_Type), u"==",
                                          wt(WeaponEnum::Knife));
                                     cand(c, ev(entity_val::kPressFB), u"!=", n(0));
                                     return &c;
                                   });
                                   return Value(cm.done());
                                 }()}}));
      nexts.push_back(make_obj({{u"id", make_arr({s(u"20"), s(u"25")})},
                                {u"facing", ff(FacingFlag::Ctrl)},
                                {u"desc", s(u"weapon swing")},
                                {u"mp_mode", n(1)},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cm.one_of(ev(entity_val::kHolding_W_Type),
                                             {wt(WeaponEnum::Knife), wt(WeaponEnum::Stick)});
                                   return Value(cm.done());
                                 }()}}));
      spread(nexts, hit_next_frame_drink());
      spread(nexts, hit_next_frame_super_punch());
      spread(nexts, hit_next_frame_punch());
      editing.hit(s(u"a"), nexts);
      editing.hit(s(u"j"), {s(u"210")});
      editing.hit(s(u"d"), {s(u"110")});
      editing.keydown(s(u"j"), {s(u"210")});
      editing.keydown(s(u"d"), {s(u"110")});
      editing.hit(s(u"FF"), {s(u"running_0")});
      frame->set(u"dvx", n(walking_speed));
      frame->set(u"dvz", n(walking_speedz));
      frame->set(u"ctrl_z", n(1));
      frame->set(u"ctrl_x", n(1));
    } else if (fid == 9 || fid == 10 || fid == 11) {
      std::vector<Value> nexts;
      nexts.push_back(make_obj({{u"id", s(u"45")},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cadd(cm, ev(entity_val::kHolding_W_Type), u"==",
                                          wt(WeaponEnum::Baseball));
                                   cm.or_([&](CondMaker& c) -> CondMaker* {
                                     cadd(c, ev(entity_val::kPressFB), u"==", n(1));
                                     cand(c, ev(entity_val::kHolding_W_Type), u"!=",
                                            wt(WeaponEnum::None));
                                     return &c;
                                   });
                                   return Value(cm.done());
                                 }()}}));
      spread(nexts, hit_next_frame_drink());
      nexts.push_back(make_obj({{u"id", s(u"35")},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cm.one_of(ev(entity_val::kHolding_W_Type),
                                             {wt(WeaponEnum::Knife), wt(WeaponEnum::Stick)});
                                   return Value(cm.done());
                                 }()}}));
      nexts.push_back(make_obj({{u"id", s(u"85")}}));
      editing.hit(s(u"a"), nexts);
      editing.hit(s(u"j"), {s(u"213")});
      editing.hit(s(u"d"), {s(u"102")});
      editing.hit(s(u"B"), {s(u"218")});
      editing.keydown(s(u"j"), {s(u"213")});
      editing.keydown(s(u"d"), {s(u"102")});
      editing.keydown(s(u"B"), {s(u"218")});
      frame->set(u"dvx", n(running_speed));
      frame->set(u"dvz", n(running_speedz));
      frame->set(u"ctrl_z", n(1));
    } else if (fid == 12 || fid == 13 || fid == 14 || fid == 15) {
      editing.hit(s(u"FF"), {s(u"heavy_obj_run_0")});
      editing.hit(s(u"a"),
                  {make_obj({{u"id", s(u"50")}, {u"facing", ff(FacingFlag::Ctrl)}})});
      frame->set(u"dvx", n(heavy_walking_speed));
      frame->set(u"dvz", n(heavy_walking_speedz));
      frame->set(u"ctrl_z", n(1));
      frame->set(u"ctrl_x", n(1));
    } else if (fid == 16 || fid == 17 || fid == 18) {
      editing.hit(s(u"a"), {s(u"50")});
      editing.hit(s(u"B"), {s(u"19")});
      editing.keydown(s(u"a"), {s(u"50")});
      editing.keydown(s(u"B"), {s(u"19")});
      frame->set(u"dvx", n(heavy_running_speed));
      frame->set(u"dvz", n(heavy_running_speedz));
      frame->set(u"ctrl_z", n(1));
    } else if (fid == 110 || fid == 111) {
      const Value bdy_v = field_at(frame_v, u"bdy");
      const Array* ba = as_array(bdy_v);
      if (ba != nullptr && ba->size() > 0) {
        for (size_t i = 0; i < ba->size(); ++i) {
          Object* bdy = as_mut(ba->at(i));
          if (bdy == nullptr) continue;
          const Value* acts = bdy->get(u"actions");
          const std::vector<Value> items = {
              make_obj({{u"type", s(u"V_BROKEN_DEFEND")},
                        {u"data", make_obj({{u"id", s(u"112")}})}}),
              make_obj({{u"type", s(u"V_DEFEND")}, {u"data", make_obj({{u"id", s(u"111")}})}})};
          Value acts_v = acts != nullptr ? *acts : Value();
          Value out = ensure(acts_v, items);
          bdy->set(u"actions", out);
        }
      }
    } else if (fid == 112) {
    } else if (fid == 210 || fid == 211 || fid == 212) {
      if (frame_id == u"211") frame->set(u"jump_flag", n(1));
      if (frame_id == u"212") add_key_down_jump_atk(frame_v);
    } else if (fid == 213 || fid == 214 || fid == 216 || fid == 217) {
      if (frame_id == u"213" && frames->get(u"214") != nullptr) {
        hit_next_frame_turn_back(frame_v, s(u"214"));
      }
      if (frame_id == u"216" && frames->get(u"217") != nullptr) {
        hit_next_frame_turn_back(frame_v, s(u"217"));
      }
      if (frame_id == u"214" && frames->get(u"213") != nullptr) {
        hit_next_frame_turn_back(frame_v, s(u"213"));
      }
      if (frame_id == u"217" && frames->get(u"216") != nullptr) {
        hit_next_frame_turn_back(frame_v, s(u"216"));
      }
      if (is_state(field_at(frame_v, u"state"), StateEnum::Dash) &&
          (frame_id == u"213" || frame_id == u"216")) {
        std::vector<Value> nexts;
        nexts.push_back(make_obj({{u"id", s(u"52")},
                                  {u"facing", ff(FacingFlag::Ctrl)},
                                  {u"expression", [&] {
                                     CondMaker cm;
                                     cm.one_of(ev(entity_val::kHolding_W_Type),
                                               {wt(WeaponEnum::Baseball), wt(WeaponEnum::Drink)});
                                     return Value(cm.done());
                                   }()}}));
        nexts.push_back(make_obj({{u"id", s(u"40")},
                                  {u"facing", ff(FacingFlag::Ctrl)},
                                  {u"expression", [&] {
                                     CondMaker cm;
                                     cm.one_of(ev(entity_val::kHolding_W_Type),
                                               {wt(WeaponEnum::Knife), wt(WeaponEnum::Stick)});
                                     return Value(cm.done());
                                   }()}}));
        nexts.push_back(make_obj({{u"id", s(u"90")}}));
        editing.keydown(s(u"a"), nexts);
      }
      if (is_state(field_at(frame_v, u"state"), StateEnum::Jump)) {
        add_key_down_jump_atk(frame_v);
        frame->set(u"state", se(StateEnum::Dash));
      }
    } else if (fid == 120 || fid == 121 || fid == 122 || fid == 123) {
      const Value cpoint_v = field_at(frame_v, u"cpoint");
      if (truthy(cpoint_v)) {
        Object* cpoint = as_mut(cpoint_v);
        if (cpoint != nullptr) {
          const Value* vaction = cpoint->get(u"vaction");
          if (vaction != nullptr && truthy(*vaction)) {
            Object* vo = as_mut(*vaction);
            if (vo != nullptr) vo->set(u"facing", ff(FacingFlag::OpposingCatcher));
          }
          const Value a_action = take(*cpoint, u"aaction");
          const Value t_action = take(*cpoint, u"taction");
          const Value s_hit_a = field_at(field_at(frame_v, u"hit"), u"a");
          std::vector<Value> t_hit_a;
          bool has_t = false;
          bool has_a = false;
          Value a_hit_a;
          if (truthy(t_action)) {
            Value nf = get_next_frame_by_raw_id(t_action, u"frame");
            Object* nfo = as_mut(nf);
            if (nfo != nullptr) {
              nfo->set(u"facing", ff(FacingFlag::Ctrl));
              CondMaker cm;
              cadd(cm, ev(entity_val::kCatching), u"==", n(1));
              cm.and_([&](CondMaker& c) -> CondMaker* {
                cadd(c, ev(entity_val::kPressFB), u"!=", n(0));
                cor(c, ev(entity_val::kPressUD), u"!=", n(0));
                return &c;
              });
              nfo->set(u"expression", Value(cm.done()));
            }
            has_t = true;
            t_hit_a.push_back(nf);
          }
          if (truthy(a_action)) {
            Value nf = get_next_frame_by_raw_id(a_action, u"frame");
            Object* nfo = as_mut(nf);
            if (nfo != nullptr) {
              CondMaker cm;
              cadd(cm, ev(entity_val::kCatching), u"==", n(1));
              nfo->set(u"expression", Value(cm.done()));
            }
            has_a = true;
            a_hit_a = nf;
          }
          if (Array* sha = arr_mut(s_hit_a)) {
            if (has_t) {
              Array rebuilt;
              for (const Value& v : t_hit_a) rebuilt.push_back(v);
              for (size_t i = 0; i < sha->size(); ++i) rebuilt.push_back(sha->at(i));
              *sha = rebuilt;
            }
            if (has_a) {
              Array rebuilt;
              rebuilt.push_back(a_hit_a);
              for (size_t i = 0; i < sha->size(); ++i) rebuilt.push_back(sha->at(i));
              *sha = rebuilt;
            }
          } else {
            int c = 0;
            if (truthy(s_hit_a)) ++c;
            if (has_t) ++c;
            if (has_a) ++c;
            if (c >= 2) {
              std::vector<Value> hit_a_list;
              if (truthy(s_hit_a)) hit_a_list.push_back(s_hit_a);
              for (const Value& v : t_hit_a) hit_a_list.push_back(v);
              if (has_a) hit_a_list.push_back(a_hit_a);
              editing.hit(s(u"a"), hit_a_list);
            } else if (c == 1) {
              if (!truthy(field_at(frame_v, u"hit"))) {
                frame->set(u"hit", Value(std::make_shared<Object>()));
              }
              if (truthy(s_hit_a)) {
                editing.hit(s(u"a"), {s_hit_a});
              } else if (has_t) {
                editing.hit(s(u"a"), t_hit_a);
              } else if (has_a) {
                editing.hit(s(u"a"), {a_hit_a});
              }
            }
          }
        }
      }
    } else if (fid == 232 || fid == 233 || fid == 234) {
      const Value vaction = field_at(field_at(frame_v, u"cpoint"), u"vaction");
      if (truthy(vaction)) {
        Object* vo = as_mut(vaction);
        if (vo != nullptr) vo->set(u"facing", ff(FacingFlag::OpposingCatcher));
      }
    } else if (fid == 100 || fid == 108) {
    } else if (fid == 180) {
    } else if (fid == 181 || fid == 182) {
      const Value* hj = key_of(as_object(field_at(frame_v, u"hit")), u"j");
      if (hj == nullptr || !truthy(*hj)) {
        editing.hit(s(u"j"), {make_obj({{u"id", s(u"100")}})});
      }
    } else if (fid == 183 || fid == 184 || fid == 185) {
    } else if (fid == 186) {
    } else if (fid == 187 || fid == 188) {
      const Value* hj = key_of(as_object(field_at(frame_v, u"hit")), u"j");
      if (hj == nullptr || !truthy(*hj)) {
        editing.hit(s(u"j"), {make_obj({{u"id", s(u"108")}})});
      }
    } else if (fid == 189 || fid == 190 || fid == 191) {
    } else if (fid == 215) {
      editing.hit(s(u"d"),
                  {make_obj({{u"id", s(u"102")}, {u"facing", ff(FacingFlag::Ctrl)}})});
      std::vector<Value> nexts;
      nexts.push_back(make_obj({{u"id", s(u"214")},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cadd(cm, ev(entity_val::kPressLR), u"==", n(0));
                                   cand(cm, ev(entity_val::kTrendX), u"==", n(-1));
                                   return Value(cm.done());
                                 }()}}));
      nexts.push_back(make_obj({{u"id", s(u"213")},
                                {u"expression", [&] {
                                   CondMaker cm;
                                   cadd(cm, ev(entity_val::kPressLR), u"!=", n(0));
                                   cor(cm, ev(entity_val::kTrendX), u"!=", n(0));
                                   return Value(cm.done());
                                 }()},
                                {u"facing", ff(FacingFlag::Trend)}}));
      editing.hit(s(u"j"), nexts);
    } else if (fid == 200) {
      frame->set(u"state", se(StateEnum::Frozen));
    } else if (fid == 220 || fid == 221 || fid == 222 || fid == 223 || fid == 224 ||
               fid == 225) {
      frame->set(u"state", se(StateEnum::Injured));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Injured));
      if (name != nullptr) frame->set(u"state_name", Value(std::u16string(u"StateEnum.") + name));
      make_frame_state(frame_v);
    } else if (fid == 226 || fid == 227 || fid == 228 || fid == 229) {
      frame->set(u"state", se(StateEnum::Tired));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Tired));
      if (name != nullptr) frame->set(u"state_name", Value(std::u16string(u"StateEnum.") + name));
      make_frame_state(frame_v);
    }

    const double state_after = to_number(field_at(frame_v, u"state"));
    if (state_after == static_cast<double>(StateEnum::Standing) ||
        state_after == static_cast<double>(StateEnum::Jump) ||
        state_after == static_cast<double>(StateEnum::Walking)) {
      hit_next_frame_turn_back(frame_v);
    } else if (state_after == static_cast<double>(StateEnum::Defend)) {
      const Array* defenders = as_array(bots_frames_defends());
      bool found = false;
      if (defenders != nullptr) {
        for (size_t i = 0; i < defenders->size(); ++i) {
          if (strict_equals(defenders->at(i), frame_id_val)) {
            found = true;
            break;
          }
        }
      }
      if (found) hit_next_frame_turn_back(frame_v);
    }

    const double state3 = to_number(field_at(frame_v, u"state"));
    if (state3 == static_cast<double>(StateEnum::Frozen)) {
      Value nexts = field_at(frame_v, u"next");
      edit_next_frame(nexts, [&frames](Value& i, size_t) {
        Object* io = as_mut(i);
        if (io == nullptr) return;
        const Value* idv = io->get(u"id");
        if (idv == nullptr || !is_str(*idv)) return;
        const std::u16string id = std::get<std::u16string>(*idv);
        const Value* fv = frames->get(id);
        if (fv == nullptr) return;
        if (!is_state(field_at(*fv, u"state"), StateEnum::Frozen)) {
          io->set(u"blink_time", n(60));
        }
      });
    } else if (state3 == static_cast<double>(StateEnum::Standing)) {
      std::vector<Value> nexts;
      spread(nexts, hit_next_frame_weapon_atk());
      spread(nexts, hit_next_frame_drink());
      spread(nexts, hit_next_frame_super_punch());
      spread(nexts, hit_next_frame_punch());
      editing.hit(s(u"a"), nexts);
      editing.hit(s(u"j"), spread_of(hit_next_frame_jump()));
      editing.hit(s(u"d"), spread_of(hit_next_frame_defend()));
      editing.hit(s(u"FF"), {s(u"running_0")});
      editing.keydown(make_arr({s(u"U"), s(u"D"), s(u"L"), s(u"R")}),
                      {make_obj({{u"id", s(u"walking_0")}, {u"facing", ff(FacingFlag::Ctrl)}})});
    } else if (state3 == static_cast<double>(StateEnum::BurnRun) ||
               state3 == static_cast<double>(StateEnum::Z_Moveable)) {
      frame->set(u"dvz", n(running_speedz));
      frame->set(u"ctrl_z", n(1));
    } else if (state3 == static_cast<double>(StateEnum::Defend)) {
      const Value bdy_v = field_at(frame_v, u"bdy");
      const Array* ba = as_array(bdy_v);
      if (ba != nullptr && ba->size() > 0) {
        for (size_t i = 0; i < ba->size(); ++i) {
          Object* bdy = as_mut(ba->at(i));
          if (bdy == nullptr) continue;
          bdy->set(u"kind", Value(static_cast<double>(BdyKind::Defend)));
          const char16_t* name = bdy_kind_name_of(static_cast<int>(BdyKind::Defend));
          if (name != nullptr) bdy->set(u"kind_name", Value(std::u16string(name)));
          const Value* acts = bdy->get(u"actions");
          const Array* acts_a = acts != nullptr ? as_array(*acts) : nullptr;
          bool has_broken = false;
          if (acts_a != nullptr) {
            for (size_t j = 0; j < acts_a->size(); ++j) {
              if (strict_equals(field_at(acts_a->at(j), u"type"), s(u"V_BROKEN_DEFEND"))) {
                has_broken = true;
                break;
              }
            }
          }
          if (!has_broken) {
            Value acts_v = acts != nullptr ? *acts : Value();
            Value out = ensure(acts_v, make_obj({{u"type", s(u"V_BROKEN_DEFEND")},
                                                 {u"data", make_obj({{u"id", s(u"112")}})}}));
            bdy->set(u"actions", out);
          }
        }
      }
    } else if (state3 == static_cast<double>(StateEnum::Walking)) {
      if (frame_id != u"12" && frame_id != u"13" && frame_id != u"14" &&
          frame_id != u"15") {
        std::vector<Value> nexts;
        spread(nexts, hit_next_frame_weapon_atk());
        spread(nexts, hit_next_frame_drink());
        spread(nexts, hit_next_frame_super_punch());
        spread(nexts, hit_next_frame_punch());
        editing.hit(s(u"a"), nexts);
        editing.hit(s(u"j"), spread_of(hit_next_frame_jump()));
        editing.hit(s(u"d"), spread_of(hit_next_frame_defend()));
        editing.hit(s(u"FF"), {s(u"running_0")});
        frame->set(u"dvx", n(walking_speed));
        frame->set(u"dvz", n(walking_speedz));
        frame->set(u"ctrl_x", n(1));
        frame->set(u"ctrl_z", n(1));
        frame->set(u"wait", n(walking_frame_rate * 2));
      }
      const std::u16string fname = to_string(field_at(frame_v, u"name"));
      const Value* existing = round_trip_frames_map.get(fname);
      if (existing == nullptr || !truthy(*existing)) {
        round_trip_frames_map.set(fname, Value(std::make_shared<Array>()));
      }
      Array* bucket = arr_mut(*round_trip_frames_map.get(fname));
      if (bucket != nullptr) bucket->push_back(frame_v);
      frames->remove(frame_id);
    } else if (state3 == static_cast<double>(StateEnum::Running)) {
      if (frame_id != u"16" && frame_id != u"17" && frame_id != u"18") {
        std::vector<Value> nexts;
        nexts.push_back(make_obj({{u"id", s(u"45")},
                                  {u"expression", [&] {
                                     CondMaker cm;
                                     cadd(cm, ev(entity_val::kHolding_W_Type), u"==",
                                            wt(WeaponEnum::Baseball));
                                     cm.or_([&](CondMaker& c) -> CondMaker* {
                                       cadd(c, ev(entity_val::kPressFB), u"==", n(1));
                                       cand(c, ev(entity_val::kHolding_W_Type), u"!=",
                                              wt(WeaponEnum::None));
                                       return &c;
                                     });
                                     return Value(cm.done());
                                   }()}}));
        spread(nexts, hit_next_frame_drink());
        nexts.push_back(make_obj({{u"id", s(u"35")},
                                  {u"facing", ff(FacingFlag::Ctrl)},
                                  {u"expression", [&] {
                                     CondMaker cm;
                                     cm.one_of(ev(entity_val::kHolding_W_Type),
                                               {wt(WeaponEnum::Knife), wt(WeaponEnum::Stick)});
                                     return Value(cm.done());
                                   }()}}));
        nexts.push_back(make_obj({{u"id", s(u"85")}}));
        editing.hit(s(u"a"), nexts);
        editing.hit(s(u"j"), {s(u"213")});
        editing.hit(s(u"d"), {s(u"102")});
        editing.keydown(s(u"B"), {s(u"218")});
        frame->set(u"dvx", n(running_speed));
        frame->set(u"dvz", n(running_speedz));
        frame->set(u"ctrl_z", n(1));
        frame->set(u"wait", n(running_frame_rate * 2));
      }
      const std::u16string fname = to_string(field_at(frame_v, u"name"));
      const Value* existing = round_trip_frames_map.get(fname);
      if (existing == nullptr || !truthy(*existing)) {
        round_trip_frames_map.set(fname, Value(std::make_shared<Array>()));
      }
      Array* bucket = arr_mut(*round_trip_frames_map.get(fname));
      if (bucket != nullptr) bucket->push_back(frame_v);
      frames->remove(frame_id);
    } else if (state3 == static_cast<double>(StateEnum::LandGoto94)) {
      frame->set(u"on_landing", make_obj({{u"id", s(u"94")}}));
    }
  }

  const std::vector<std::u16string> map_keys = round_trip_frames_map.keys();
  for (const std::u16string& prefix : map_keys) {
    const Value* sv = round_trip_frames_map.get(prefix);
    const Array* src = sv != nullptr ? as_array(*sv) : nullptr;
    if (src == nullptr) continue;
    const size_t src_len = src->size();
    for (size_t i = 0; i < 2 * src_len - 2; ++i) {
      Value frame_v;
      if (i < src_len) {
        frame_v = src->at(i);
      } else {
        const Object* ro = as_object(src->at(2 * (src_len - 1) - i));
        frame_v = Value(std::make_shared<Object>(ro != nullptr ? *ro : Object()));
      }
      Object* frame = as_mut(frame_v);
      if (frame == nullptr) continue;
      const std::u16string new_id =
          prefix + std::u16string(u"_") + number_to_string(static_cast<double>(i));
      frame->set(u"id", Value(new_id));
      const double next_index = static_cast<double>(i == 2 * src_len - 3 ? 0 : i + 1);
      Value next_obj = make_obj({{u"id", Value(prefix + std::u16string(u"_") +
                                               number_to_string(next_index))}});
      const double fstate = to_number(field_at(frame_v, u"state"));
      if (fstate == static_cast<double>(StateEnum::Standing) ||
          fstate == static_cast<double>(StateEnum::Walking)) {
        as_mut(next_obj)->set(u"facing", ff(FacingFlag::Ctrl));
      }
      frame->set(u"next", next_obj);
      frames->set(new_id, frame_v);
    }
  }

  Object indexes;
  indexes.set(u"heavy_obj_walk", s(u"heavy_obj_walk_0"));
  indexes.set(u"ice", s(u"200"));
  indexes.set(u"fire", make_arr({s(u"203"), s(u"205")}));
  indexes.set(u"injured", make_obj({{u"-1", s(u"220")}, {u"1", s(u"222")}}));
  indexes.set(u"dizzy", s(u"226"));
  indexes.set(u"lying", make_obj({{u"-1", s(u"230")}, {u"1", s(u"231")}}));
  indexes.set(u"grand_injured",
              make_obj({{u"-1", make_arr({s(u"220")})}, {u"1", make_arr({s(u"222")})}}));
  indexes.set(u"in_the_skys", make_arr({s(u"212")}));
  indexes.set(u"critical_hit",
              make_obj({{u"-1", make_arr({s(u"180")})}, {u"1", make_arr({s(u"186")})}}));
  indexes.set(u"falling",
              make_obj({{u"-1", make_arr({s(u"181"), s(u"182"), s(u"183")})},
                        {u"1", make_arr({s(u"187"), s(u"188"), s(u"189")})}}));
  indexes.set(u"bouncing", make_obj({{u"-1", make_arr({s(u"184"), s(u"185")})},
                                     {u"1", make_arr({s(u"190"), s(u"191")})}}));
  indexes.set(u"landing_1", s(u"215"));
  indexes.set(u"landing_2", s(u"219"));

  Value ret_v = make_obj({{u"id", field_at(dat_index, u"id")},
                          {u"type", Value(static_cast<double>(EntityEnum::Fighter))},
                          {u"base", *base_v},
                          {u"indexes", Value(std::make_shared<Object>(indexes))},
                          {u"frames", *frames_v},
                          {u"processed", Value(false)}});
  cook_transform_begin_expression_to_hit(*frames_v);
  cook_file_variants(ret_v);
  if (truthy(field_at(dat_index, u"bot"))) {
    base->set(u"bot_id", field_at(dat_index, u"bot"));
  }
  return ret_v;
}

static void cook_transform_begin_expression_to_hit(Value& frames_v) {
  Object* frames = as_mut(frames_v);
  if (frames == nullptr) return;
  Object tframes;
  const std::vector<std::u16string> keys = frames->keys();
  for (const std::u16string& k : keys) {
    const Value* fv = frames->get(k);
    if (fv == nullptr) continue;
    if (is_state(field_at(*fv, u"state"), StateEnum::TransformToCatching_Begin)) {
      tframes.set(to_string(field_at(*fv, u"id")), *fv);
    }
  }
  if (tframes.keys().empty()) return;

  for (const std::u16string& k : keys) {
    const Value* fv = frames->get(k);
    if (fv == nullptr) continue;
    Object* fo = as_mut(*fv);
    if (fo == nullptr) continue;
    const char16_t* slots[4] = {u"key_down", u"key_up", u"hit", u"seqs"};
    for (const char16_t* slot : slots) {
      const Value* sv = fo->get(std::u16string(slot));
      if (sv == nullptr) continue;
      const Object* so = as_object(*sv);
      if (so == nullptr) continue;
      const std::vector<std::u16string> sub = so->keys();
      for (const std::u16string& sk : sub) {
        const Value* item = so->get(sk);
        if (item == nullptr) continue;
        cook_marker(*item, tframes);
      }
    }
  }
}

}
}
