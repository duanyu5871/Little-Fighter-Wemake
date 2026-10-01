#include "value.h"

#include <cmath>

namespace lfw {

bool truthy(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return false;
  if (std::holds_alternative<NullTag>(v)) return false;
  if (const bool* b = std::get_if<bool>(&v)) return *b;
  if (const double* d = std::get_if<double>(&v)) return *d != 0.0 && !std::isnan(*d);
  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return !s->empty();
  return true;
}

const char* type_of(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return "undefined";
  if (std::holds_alternative<NullTag>(v)) return "object";
  if (std::holds_alternative<bool>(v)) return "boolean";
  if (std::holds_alternative<double>(v)) return "number";
  if (std::holds_alternative<std::u16string>(v)) return "string";
  return "object";
}

const Array* as_array(const Value& v) {
  const std::shared_ptr<Array>* p = std::get_if<std::shared_ptr<Array>>(&v);
  return p && *p ? p->get() : nullptr;
}

Array* as_array(Value& v) {
  std::shared_ptr<Array>* p = std::get_if<std::shared_ptr<Array>>(&v);
  return p && *p ? p->get() : nullptr;
}

bool is_array(const Value& v) { return as_array(v) != nullptr; }

}
