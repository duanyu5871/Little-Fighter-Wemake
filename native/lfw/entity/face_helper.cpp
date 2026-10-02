#include "lfw/entity/face_helper.h"

#include <variant>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace entity {

double same_face(const Value& ref, const Value& target) {
  return strict_equals(field_or(ref, u"facing"), field_or(target, u"facing")) ? 1.0 : -1.0;
}

Value turn_face(const Value& f) {
  if (std::holds_alternative<std::monostate>(f)) return Value();
  return strict_equals(f, Value(1.0)) ? Value(-1.0) : Value(1.0);
}

}
}
