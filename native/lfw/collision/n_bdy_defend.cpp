#include "lfw/collision/n_bdy_defend.h"

#include <variant>

#include "lfw/collision/handlers.h"
#include "lfw/collision/handlers2.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/collision_defaults.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/spark_enum.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace collision {
namespace {

NbdDefendEnv g_env;

bool missing(const Value& v) { return std::holds_alternative<std::monostate>(v); }

bool is_effect(const Value& effect, ItrEffect want) {
  return strict_equals(effect, Value(static_cast<double>(want)));
}

void run_actions(const Value& actions, const char16_t* a_type, const char16_t* v_type,
                 const char16_t* a_dispatch, const char16_t* v_dispatch) {
  const Array* arr = as_array(actions);
  if (arr == nullptr) return;
  for (size_t i = 0; i < arr->size(); ++i) {
    const Value& action = arr->at(i);
    const Value type = field_or(action, u"type");
    if (strict_equals(type, Value(std::u16string(a_type)))) {
      g_env.dispatch(a_dispatch, action);
    }
    if (strict_equals(type, Value(std::u16string(v_type)))) {
      g_env.dispatch(v_dispatch, action);
    }
  }
}

}

const NbdDefendEnv& nbd_defend_env() { return g_env; }
void set_nbd_defend_env(const NbdDefendEnv& env) { g_env = env; }

void handle_itr_normal_bdy_defend(Collision& c) {
  INdbdyDefendEntity* a = g_env.find_entity(c.aid);
  INdbdyDefendEntity* v = g_env.find_entity(c.vid);
  if (a == nullptr || v == nullptr) return;

  const Value effect_v = field_or(c.itr, u"effect");
  const Value bdefend_v = field_or(c.itr, u"bdefend");
  const double bdefend = missing(bdefend_v) ? kDefaultBreakDefendValue : to_number(bdefend_v);

  const bool explosive =
      is_effect(effect_v, ItrEffect::FireExplosion) || is_effect(effect_v, ItrEffect::Explosion);
  if ((!explosive && strict_equals(a->facing(), v->facing())) ||
      bdefend >= kDefaultForceBreakDefendValue) {
    handle_itr_normal_bdy_normal(c);
    return;
  }

  v->set_defend_value(Value(to_number(v->defend_value()) - bdefend));
  handle_injury(c, to_number(v->defend_ratio()), false);
  handle_rest(c);
  handle_stiffness(c);

  double x = 0;
  double y = 0;
  double z = 0;
  v->spark_point(c.a_cube, c.b_cube, x, y, z);
  const ItrVelocity iv = g_env.calc_velocity(c);
  if (truthy(Value(iv.x))) v->set_velocity(Value(iv.x / 2.0), Value(), Value());

  const Value itr_actions = field_or(c.itr, u"actions");
  const Value bdy_actions = field_or(c.bdy, u"actions");

  if (to_number(v->defend_value()) <= 0) {
    v->set_defend_value(Value(0.0));
    g_env.spark(Value(x), Value(y), Value(z), Value(std::u16string(spark_enum::kBrokenDefend)));
    run_actions(itr_actions, action_type::kA_DEFEND, action_type::kV_DEFEND,
                action_type::kA_NEXT_FRAME, action_type::kV_NEXT_FRAME);
    run_actions(bdy_actions, action_type::kA_BROKEN_DEFEND, action_type::kV_BROKEN_DEFEND,
                action_type::kA_NEXT_FRAME, action_type::kV_NEXT_FRAME);
  } else {
    g_env.spark(Value(x), Value(y), Value(z), Value(std::u16string(spark_enum::kDefendHit)));
    run_actions(itr_actions, action_type::kA_DEFEND, action_type::kV_DEFEND,
                action_type::kA_NEXT_FRAME, action_type::kV_NEXT_FRAME);
    run_actions(bdy_actions, action_type::kA_DEFEND, action_type::kV_DEFEND,
                action_type::kA_NEXT_FRAME, action_type::kV_NEXT_FRAME);
  }
}

}
}
