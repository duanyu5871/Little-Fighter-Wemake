#include "lfw/helper/manhattan_xz.h"

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/round_float.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace helper {

double manhattan_xz(const Value& a, const Value& b) {
  const Value pa = field_or(a, u"position");
  const Value pb = field_or(b, u"position");
  const double dx = to_number(field_or(pa, u"x")) - to_number(field_or(pb, u"x"));
  const double dz = to_number(field_or(pa, u"z")) - to_number(field_or(pb, u"z"));
  return round_float(abs(dx) + abs(dz));
}

}
}
