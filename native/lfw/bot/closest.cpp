#include "lfw/bot/closest.h"

#include <cstddef>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace bot {

Value closest(const Value& me, const std::vector<Value>& list) {
  const Value pm = field_or(me, u"position");
  Value ret;
  double distance = 0;
  for (const Value& it : list) {
    if (!truthy(it)) continue;
    const Value pi = field_or(it, u"position");
    const double dx = to_number(field_or(pm, u"x")) - to_number(field_or(pi, u"x"));
    const double dz = to_number(field_or(pm, u"z")) - to_number(field_or(pi, u"z"));
    const double d = round(abs(dx) + abs(dz));
    if (!truthy(ret) || d < distance) {
      ret = it;
      distance = d;
    }
  }
  return ret;
}

}
}
