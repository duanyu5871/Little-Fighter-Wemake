#include "lfw/entity/find_frame_direction.h"

#include <cstddef>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace entity {
namespace {

bool array_contains(const Value& v, const Value& needle) {
  const Array* a = as_array(v);
  if (a == nullptr) return false;
  for (size_t i = 0; i < a->size(); ++i) {
    if (equals(a->at(i), needle)) return true;
  }
  return false;
}

}

double find_direction(const Value& frame, const Value& pair) {
  if (!truthy(pair)) return 0.0;
  const Value id = field_or(frame, u"id");
  const Value a = field_or(pair, u"-1");
  const Value b = field_or(pair, u"1");
  if (equals(a, id) || array_contains(a, id)) return -1.0;
  if (equals(b, id) || array_contains(b, id)) return 1.0;
  return 0.0;
}

}
}
