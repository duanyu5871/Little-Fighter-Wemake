#include "lfw/utils/js_add.h"

#include <string>
#include <variant>

#include "lfw/core/value.h"

namespace lfw {

Value js_add(const Value& a, const Value& b) {
  const Value pa = to_primitive(a);
  const Value pb = to_primitive(b);
  if (std::holds_alternative<std::u16string>(pa) || std::holds_alternative<std::u16string>(pb)) {
    return Value(to_string(pa) + to_string(pb));
  }
  return Value(to_number(pa) + to_number(pb));
}

}
