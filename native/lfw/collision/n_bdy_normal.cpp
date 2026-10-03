#include "lfw/collision/n_bdy_normal.h"

#include <cmath>
#include <optional>

#include "lfw/collision/handlers.h"
#include "lfw/collision/handlers2.h"
#include "lfw/collision/handlers3.h"
#include "lfw/defines/collision_defaults.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/spark_enum.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace collision {
namespace {

NbdyNormalEnv g_env;

bool effect_is(const Value& effect, ItrEffect want) {
  return strict_equals(effect, Value(static_cast<double>(want)));
}

double hurt_frame_group(double r) {
  const double a = std::fmod(lfw::floor(r / 50.0), 2.0);
  return a > 0 ? 1.0 : -1.0;
}

std::optional<double> value_length(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  if (s != nullptr) return static_cast<double>(s->size());
  const Array* arr = as_array(v);
  if (arr != nullptr) return static_cast<double>(arr->size());
  return std::nullopt;
}

Value index_by(const Value& holder, const std::u16string& k) {
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(k);
    if (p != nullptr) return *p;
  }
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    const double index = to_number(Value(k));
    if (!(index >= 0) || index >= static_cast<double>(arr->size())) return Value();
    return arr->at(static_cast<size_t>(index));
  }
  return Value();
}

void apply_injury_rest_stiffness_fall(Collision& c) {
  handle_injury(c, 1.0, false);
  handle_rest(c);
  handle_stiffness(c);
  handle_fall(c);
}

void apply_injury_stiffness_rest_fall(Collision& c) {
  handle_injury(c, 1.0, false);
  handle_stiffness(c);
  handle_rest(c);
  handle_fall(c);
}

void apply_normal_impact(Collision& c, const Value& effect_v) {
  handle_injury(c, 1.0, false);
  handle_rest(c);
  handle_stiffness(c);

  INbdyNormalEntity* a = g_env.find_entity(c.aid);
  INbdyNormalEntity* v = g_env.find_entity(c.vid);
  if (a == nullptr || v == nullptr) return;

  v->set_fall_value(Value(to_number(v->fall_value()) - to_number(a->itr_fall(c.itr))));
  v->set_defend_value(Value(0.0));
  if (g_env.is_fall(c)) {
    handle_fall(c);
    return;
  }

  const ItrVelocity iv = g_env.calc_velocity(c);
  v->set_velocity(Value(iv.x), Value(iv.y), Value(iv.z));

  double sx = 0;
  double sy = 0;
  double sz = 0;
  v->spark_point(c.a_cube, c.b_cube, sx, sy, sz);

  const bool v_is_fighter = g_env.is_fighter(*v);
  if (effect_is(effect_v, ItrEffect::Sharp) && v_is_fighter) {
    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kBleed)));
  } else if (v_is_fighter) {
    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kHit)));
  } else {
    g_env.spark(Value(sx), Value(sy), Value(sz), Value(std::u16string(spark_enum::kSilentHit)));
  }

  const bool ic = strict_equals(v->state(), Value(static_cast<double>(StateEnum::Caught)));
  const Value backhurtact = v->cpoint_backhurtact();
  const Value fronthurtact = v->cpoint_fronthurtact();
  const Value fvm = v->fall_value_max();
  const Value fv = v->fall_value();
  const Value dizzy = v->data_indexes_dizzy();
  const Value grand_injured = v->data_indexes_grand_injured();
  const Value injured = v->data_indexes_injured();
  const bool on_hurt_side = v->position_y() <= v->ground_y();
  const Value hurt_list = on_hurt_side ? grand_injured : injured;
  const double r = to_number(fvm) - to_number(fv);
  const double d = kDefaultFallValueDizzy;
  const bool same_face = strict_equals(a->facing(), v->facing());

  Value id;
  if (ic) {
    id = same_face ? backhurtact : fronthurtact;
  } else if (to_number(fv) <= d) {
    id = dizzy;
  } else {
    id = index_by(hurt_list, to_string(Value(hurt_frame_group(r))));
  }

  if (truthy(id)) {
    const std::optional<double> len = value_length(id);
    if (len.has_value() && *len > 0) v->enter_frame_by_id(id);
  }
}

}

const NbdyNormalEnv& nbdy_normal_env() { return g_env; }
void set_nbdy_normal_env(const NbdyNormalEnv& env) { g_env = env; }

void handle_itr_normal_bdy_normal(Collision& c) {
  const Value effect_v = field_or(c.itr, u"effect");
  if (equals(effect_v, Value(static_cast<double>(ItrEffect::Ignore)))) return;
  if (handle_armor(c)) return;

  if (effect_is(effect_v, ItrEffect::Fire) || effect_is(effect_v, ItrEffect::MFire1) ||
      effect_is(effect_v, ItrEffect::MFire2) || effect_is(effect_v, ItrEffect::FireExplosion)) {
    apply_injury_rest_stiffness_fall(c);
    return;
  }
  if (effect_is(effect_v, ItrEffect::Ice2)) {
    handle_itr_effect_freeze(c);
    return;
  }
  if (effect_is(effect_v, ItrEffect::Ice)) {
    const INbdyNormalEntity* v = g_env.find_entity(c.vid);
    const bool frozen =
        v != nullptr && strict_equals(v->state(), Value(static_cast<double>(StateEnum::Frozen)));
    if (frozen) {
      apply_injury_stiffness_rest_fall(c);
    } else {
      handle_itr_effect_freeze(c);
    }
    return;
  }
  if (effect_is(effect_v, ItrEffect::Explosion) || effect_is(effect_v, ItrEffect::Normal) ||
      effect_is(effect_v, ItrEffect::Sharp) || std::holds_alternative<std::monostate>(effect_v)) {
    apply_normal_impact(c, effect_v);
    return;
  }
}

}
}
