#include "lfw/collision/stiffness.h"

#include <variant>

#include "lfw/core/value.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/entity/entity_dataset.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace collision {
namespace {

Value or_default(const Value& v, const Value& fallback) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v) ? fallback
                                                                                        : v;
}

Value motionless_of(const Value& attacker) {
  const Value type = field_or(field_or(attacker, u"data"), u"type");
  const bool is_ball =
      strict_equals(type, Value(static_cast<double>(EntityEnum::Ball)));
  return entity::entity_dataset(attacker,
                               is_ball ? u"ball_itr_motionless" : u"itr_motionless");
}

Value shaking_of(const Value& attacker) {
  return field_or(field_or(field_or(attacker, u"world"), u"dataset"), u"itr_shaking");
}

}

Stiffness calc_stiffness(const Value& collision) {
  const Value itr = field_or(collision, u"itr");
  const Value attacker = field_or(collision, u"attacker");
  Stiffness out{};
  out.motionless = or_default(field_or(itr, u"motionless"), motionless_of(attacker));
  out.shaking = or_default(field_or(itr, u"shaking"), shaking_of(attacker));
  return out;
}

}
}
