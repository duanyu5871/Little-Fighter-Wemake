#include "lfw/collision/calc_itr_velocity.h"

#include <variant>

#include "lfw/collision/is_fall.h"
#include "lfw/core/value.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/entity_dataset.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace collision {
namespace {

double number_or(const Value& v, double fallback) {
  return std::holds_alternative<std::monostate>(v) ? fallback : to_number(v);
}

double weight_of(const Value& victim) {
  const Value w = field_or(field_or(field_or(victim, u"data"), u"base"), u"weight");
  if (std::holds_alternative<std::monostate>(w) || std::holds_alternative<NullTag>(w)) return 1.0;
  return to_number(w);
}

double x_of(const Value& entity) {
  return to_number(field_or(field_or(entity, u"position"), u"x"));
}

}

ItrVelocity calc_itr_velocity(const Value& collision) {
  const Value itr = field_or(collision, u"itr");
  const Value attacker = field_or(collision, u"attacker");
  const Value victim = field_or(collision, u"victim");

  const double dvx = number_or(field_or(itr, u"dvx"), 0.0);
  const Value dvy_raw = field_or(itr, u"dvy");
  const double dvy = std::holds_alternative<std::monostate>(dvy_raw)
                         ? to_number(entity::entity_dataset(attacker, u"ivy_d"))
                         : to_number(dvy_raw);
  const double dvz = number_or(field_or(itr, u"dvz"), 0.0);

  const double diff_x = x_of(victim) - x_of(attacker);
  const double weight = weight_of(victim);

  const bool position_based =
      equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::FireExplosion))) ||
      equals(field_or(itr, u"effect"), Value(static_cast<double>(ItrEffect::Explosion))) ||
      strict_equals(field_or(field_or(attacker, u"frame"), u"state"),
                    Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)));

  Value x_direction(-1.0);
  if (!position_based) {
    x_direction = field_or(attacker, u"facing");
  } else if (diff_x > 0.0) {
    x_direction = Value(-1.0);
  } else if (diff_x < 0.0) {
    x_direction = Value(1.0);
  }

  ItrVelocity out{};
  out.x = dvx * to_number(entity::entity_dataset(attacker, u"ivx_f")) * to_number(x_direction) /
          weight;
  out.y =
      (is_fall(collision) ? dvy * to_number(entity::entity_dataset(attacker, u"ivy_f")) : 0.0) /
      weight;
  out.z = dvz * to_number(entity::entity_dataset(attacker, u"ivz_f")) / weight;
  out.x_direction = x_direction;
  return out;
}

}
}
