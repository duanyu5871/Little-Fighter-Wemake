#include "lfw/dat_translator/cookers.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/dat_translator/helpers.h"
#include "lfw/dat_translator/next_frame.h"
#include "lfw/defines/c_point_kind.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/labels.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/fields.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

void set_opt(Object& o, const char16_t* key, const std::optional<double>& v) {
  o.set(std::u16string(key), v.has_value() ? Value(*v) : Value());
}

bool state_is(const Value& frame, StateEnum want) {
  const Object* fo = as_object(frame);
  if (fo == nullptr) return false;
  const Value* s = fo->get(u"state");
  return s != nullptr && strict_equals(*s, Value(static_cast<double>(want)));
}

bool is_hardcoded_action(const Value* action) {
  if (action == nullptr || as_array(*action) != nullptr) return false;
  const Object* ao = as_object(*action);
  if (ao == nullptr) return false;
  const Value* idv = ao->get(u"id");
  if (idv == nullptr || !is_str(*idv)) return false;
  const std::u16string id = std::get<std::u16string>(*idv);
  return id == u"50" || id == u"54" || id == u"109";
}

Value shallow_clone(const Value& v) {
  const Object* o = as_object(v);
  if (o == nullptr) return v;
  Object out;
  for (const std::u16string& k : o->keys()) {
    const Value* x = o->get(k);
    if (x != nullptr) out.set(k, *x);
  }
  return Value(std::make_shared<Object>(out));
}

void put_or_keep(Object& o, const char16_t* key, const Value& v) {
  if (truthy(v)) o.set(std::u16string(key), v);
}

}

void cook_bdy(Value& bdy, const Value& frame) {
  (void)frame;
  Object* o = as_object(bdy);
  if (o == nullptr) return;
  const double kind = to_number(take(*o, u"kind"));
  set_bdy_kind(*o, Value(kind));
  reorder_fields(bdy, bdy_info_fields());
}

void cook_wpoint(Value& wpoint, const Value& frame) {
  (void)frame;
  Object* o = as_object(wpoint);
  if (o == nullptr) return;

  const Value x = take(*o, u"x");
  o->set(u"x", truthy(x) ? x : Value(0.0));
  const Value y = take(*o, u"y");
  o->set(u"y", truthy(y) ? y : Value(0.0));
  const Value z = take(*o, u"z");
  o->set(u"z", truthy(z) ? z : Value(0.0));

  put_or_keep(*o, u"dvx", take(*o, u"dvx"));
  put_or_keep(*o, u"dvz", take(*o, u"dvz"));
  put_or_keep(*o, u"dvy", take(*o, u"dvy"));

  const Value attacking = take(*o, u"attacking");
  if (truthy(attacking)) o->set(u"attacking", Value(to_string(attacking)));

  o->set(u"z", Value(0.0));
  const Value* kindv = o->get(u"kind");
  if (kindv != nullptr && equals(*kindv, Value(1.0))) {
    const std::optional<double> cover =
        take_not_zero_num(*o, u"cover", [](double n) { return n; });
    o->set(u"z", Value(2.0));
  }

  o->set(u"weaponact", Value(to_string(take(*o, u"weaponact"))));
}

void cook_cpoint(Value& cpoint, const Value& frame) {
  (void)frame;
  Object* o = as_object(cpoint);
  if (o == nullptr) return;

  set_opt(*o, u"x", take_not_zero_num(*o, u"x"));
  set_opt(*o, u"y", take_not_zero_num(*o, u"y"));
  set_opt(*o, u"z", take_not_zero_num(*o, u"z"));
  set_opt(*o, u"throwvx", take_not_zero_num(*o, u"throwvx"));
  set_opt(*o, u"throwvy", take_not_zero_num(*o, u"throwvy"));
  set_opt(*o, u"throwvz", take_not_zero_num(*o, u"throwvz"));
  set_opt(*o, u"throwinjury", take_not_zero_num(*o, u"throwinjury"));
  set_opt(*o, u"decrease", take_num(*o, u"decrease", [](double n) { return -abs(n); }));

  const std::optional<double> cover = take_not_zero_num(*o, u"cover");
  if (cover.has_value() && (*cover == 1.0 || *cover == 11.0)) o->set(u"z", Value(1.0));
  if (cover.has_value() && (*cover == 0.0 || *cover == 10.0)) o->set(u"z", Value(-1.0));
  const Value* kindv = o->get(u"kind");
  if (!cover.has_value() && kindv != nullptr &&
      equals(Value(static_cast<double>(CPointKind::Attacker)), *kindv)) {
    o->set(u"z", Value(-1.0));
  }

  const Value vaction = take(*o, u"vaction");
  const std::optional<double> injury = take_num(*o, u"injury");
  if (injury.has_value() && truthy(Value(*injury))) o->set(u"injury", Value(abs(*injury)));
  if (injury.has_value() && truthy(Value(*injury)) && *injury > 0) o->set(u"shaking", Value(2.0));
  if (injury.has_value() && truthy(Value(*injury)) && *injury > 0) o->set(u"motionless", Value(2.0));

  if (is_str(vaction) || is_num(vaction)) {
    Value nf = get_next_frame_by_raw_id(vaction, u"frame", u"", nullptr);
    if (cover.has_value() && (*cover == 11.0 || *cover == 10.0)) {
      if (Object* nfo = as_object(nf)) {
        nfo->set(u"facing", Value(static_cast<double>(FacingFlag::SameAsCatcher)));
      }
    }
    const Value* throwvx = o->get(u"throwvx");
    if (throwvx != nullptr && truthy(*throwvx)) {
      if (Object* nfo = as_object(nf)) nfo->remove(u"facing");
    }
    o->set(u"vaction", nf);
  }
}

void cook_itr(Value& itr, const Value& frame) {
  (void)frame;
  Object* o = as_object(itr);
  if (o == nullptr) return;

  set_opt(*o, u"vrest", take_positive_num(*o, u"vrest", [](double n) { return 2 * n; }));
  set_opt(*o, u"arest", take_positive_num(*o, u"arest", [](double n) { return 2 * n; }));
  set_opt(*o, u"dvx", take_not_zero_num(*o, u"dvx", [](double n) { return fixed_float(n, 4); }));
  set_opt(*o, u"dvz", take_not_zero_num(*o, u"dvz", [](double n) { return fixed_float(n, 4); }));
  set_opt(*o, u"dvy", take_not_zero_num(*o, u"dvy", [](double n) { return fixed_float(n, 4); }));
  set_opt(*o, u"fall", take_not_zero_num(*o, u"fall", [](double n) { return n * 2; }));
  set_opt(*o, u"bdefend", take_not_zero_num(*o, u"bdefend", [](double n) { return n * 2; }));

  const std::optional<double> zwidth = take_not_zero_num(*o, u"zwidth");
  if (zwidth.has_value() && not_zero_num(Value(*zwidth))) {
    o->set(u"l", Value(4 * *zwidth));
    o->set(u"z", Value(-2 * *zwidth));
  }

  const Value* kindv = o->get(u"kind");
  const Value kind_name = defines::js_enum_get(u"ItrKind", kindv != nullptr ? *kindv : Value());
  if (truthy(kind_name)) o->set(u"kind_name", Value(u"ItrKind." + to_string(kind_name)));

  const Value* eff = o->get(u"effect");
  if (eff != nullptr && !std::holds_alternative<std::monostate>(*eff)) {
    const Value effect_name = defines::js_enum_get(u"ItrEffect", *eff);
    if (truthy(effect_name)) o->set(u"effect_name", Value(u"ItrEffect." + to_string(effect_name)));
  }

  const Value catchingact = take(*o, u"catchingact");
  if (is_num(catchingact)) {
    o->set(u"catchingact", get_next_frame_by_raw_id(catchingact, u"frame", u"", nullptr));
  }

  const Value caughtact = take(*o, u"caughtact");
  if (is_num(caughtact)) {
    Value nf = shallow_clone(get_next_frame_by_raw_id(caughtact, u"frame", u"", nullptr));
    if (Object* nfo = as_object(nf)) {
      nfo->set(u"facing", Value(static_cast<double>(FacingFlag::OpposingCatcher)));
    }
    o->set(u"caughtact", nf);
  }
}

void float_scaling_itr(Value& v) {
  Object* o = as_object(v);
  if (o == nullptr) return;
  static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz"};
  for (const char16_t* k : kKeys) {
    const Value* x = o->get(std::u16string(k));
    if (x != nullptr && is_num(*x)) {
      o->set(std::u16string(k), Value(js_floor(10000 * std::get<double>(*x))));
    }
  }
}

void cook_opoint(Value& opoint, const Value& frame) {
  Object* o = as_object(opoint);
  if (o == nullptr) return;

  const Value raw_action = take(*o, u"action");
  o->set(u"oid", Value(to_string(take(*o, u"oid"))));

  if (is_num(raw_action)) {
    Value act = get_next_frame_by_raw_id(raw_action, u"frame", u"", nullptr);
    const Value facing = take(*o, u"facing");
    double face = static_cast<double>(FacingFlag::None);
    if (is_num(facing)) {
      const double f = std::get<double>(facing);
      face = std::fmod(f, 2.0) != 0 ? static_cast<double>(FacingFlag::Backward)
                                    : static_cast<double>(FacingFlag::None);
      if (f >= 2 && f <= 19) face = static_cast<double>(FacingFlag::Right);
      else if (f >= 20) o->set(u"multi", Value(js_round(f / 10)));
    }
    if (Object* ao = as_object(act)) ao->set(u"facing", Value(face));
    o->set(u"action", act);
  }

  const Value dvx = take(*o, u"dvx");
  if (not_zero_num(dvx)) o->set(u"dvx", Value(std::get<double>(dvx) * 0.5));
  else o->set(u"dvx", Value(0.0));

  const Value dvz = take(*o, u"dvz");
  if (not_zero_num(dvz)) o->set(u"dvz", Value(std::get<double>(dvz) * 0.5));

  const Value dvy = take(*o, u"dvy");
  if (not_zero_num(dvy)) o->set(u"dvy", Value(std::get<double>(dvy) * -0.5));

  if (state_is(frame, StateEnum::Ball_Flying) || state_is(frame, StateEnum::Ball_3006) ||
      state_is(frame, StateEnum::Weapon_Throwing) ||
      state_is(frame, StateEnum::HeavyWeapon_InTheSky)) {
    o->set(u"speedz", Value(defines::num(u"Defines.DEFAULT_OPOINT_SPEED_Z")));
  }

  const Value* oidv = o->get(u"oid");
  const std::u16string oid = oidv != nullptr ? to_string(*oidv) : std::u16string();
  if (oid == std::u16string(oid::kFirenFlame)) {
    const bool hardcoded = is_hardcoded_action(o->get(u"action"));
    o->set(u"speedz", hardcoded ? Value(0.0)
                                 : Value(defines::num(u"Defines.DEFAULT_FIREN_FLAME_SPEED_Z")));
  } else if (oid == std::u16string(oid::kHenryWind) || oid == std::u16string(oid::kFirzenBall) ||
             oid == std::u16string(oid::kBat) || oid == std::u16string(oid::kBatChase) ||
             oid == std::u16string(oid::kBatBall) || oid == std::u16string(oid::kJanChase) ||
             oid == std::u16string(oid::kJanChaseh)) {
    o->set(u"speedz", Value(0.0));
  }
}

}

}
