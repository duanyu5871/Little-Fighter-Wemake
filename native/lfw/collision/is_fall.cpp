#include "lfw/collision/is_fall.h"

#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace collision {
namespace {

bool entity_state_is(const Value& entity, StateEnum want) {
  return strict_equals(field_or(field_or(entity, u"frame"), u"state"),
                       Value(static_cast<double>(want)));
}

}

bool is_fall(const Value& collision) {
  const Value victim = field_or(collision, u"victim");
  if (!entity::is_fighter(victim)) return true;
  const Value fall_value = field_or(victim, u"fall_value");
  if (le(fall_value, Value(0.0))) return true;
  if (le(field_or(victim, u"hp"), Value(0.0))) return true;
  if (entity_state_is(victim, StateEnum::Frozen)) return true;
  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_DIZZY"))) &&
      entity_state_is(victim, StateEnum::Caught)) {
    return true;
  }
  if (lt(fall_value, Value(defines::num(u"Defines.DEFAULT_FALL_VALUE_CRITICAL"))) &&
      !truthy(field_or(victim, u"is_on_ground"))) {
    return true;
  }
  return false;
}

}
}
