#include "lfw/helper/closer_one.h"

#include "lfw/core/value.h"
#include "lfw/helper/manhattan_xz.h"

namespace lfw {
namespace helper {

Value closer_one(const Value& s, const Value& t1, const Value& t2) {
  if (!truthy(s)) return Value();
  if (truthy(t1) && truthy(t2)) {
    return manhattan_xz(s, t1) > manhattan_xz(s, t2) ? t2 : t1;
  }
  if (truthy(t1)) return t1;
  if (truthy(t2)) return t2;
  return Value();
}

}
}
