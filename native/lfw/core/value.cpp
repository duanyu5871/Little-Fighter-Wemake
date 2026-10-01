#include "value.h"

#include <cmath>
#include <limits>

#include "js_string.h"

namespace lfw {

namespace {

enum VKind { V_UNDEF, V_NULL, V_BOOL, V_NUM, V_STR, V_ARR };

VKind kind_of(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return V_UNDEF;
  if (std::holds_alternative<NullTag>(v)) return V_NULL;
  if (std::holds_alternative<bool>(v)) return V_BOOL;
  if (std::holds_alternative<double>(v)) return V_NUM;
  if (std::holds_alternative<std::u16string>(v)) return V_STR;
  return V_ARR;
}

}

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

std::u16string array_join(const Array& a) {
  std::u16string out;
  for (size_t i = 0; i < a.size(); ++i) {
    if (i != 0) out.push_back(u',');
    const Value& item = a.at(i);
    if (std::holds_alternative<std::monostate>(item)) continue;
    if (std::holds_alternative<NullTag>(item)) continue;
    out += to_string(item);
  }
  return out;
}

std::u16string to_string(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return u"undefined";
  if (std::holds_alternative<NullTag>(v)) return u"null";
  if (const bool* b = std::get_if<bool>(&v)) return *b ? u"true" : u"false";
  if (const double* d = std::get_if<double>(&v)) return number_to_string(*d);
  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return *s;
  return array_join(*as_array(v));
}

Value to_primitive(const Value& v) {
  if (const Array* a = as_array(v)) return Value(array_join(*a));
  return v;
}

double to_number(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return std::numeric_limits<double>::quiet_NaN();
  if (std::holds_alternative<NullTag>(v)) return 0.0;
  if (const bool* b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
  if (const double* d = std::get_if<double>(&v)) return *d;
  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return string_to_number(*s);
  return string_to_number(array_join(*as_array(v)));
}

bool strict_equals(const Value& a, const Value& b) {
  if (std::holds_alternative<std::monostate>(a)) return std::holds_alternative<std::monostate>(b);
  if (std::holds_alternative<std::monostate>(b)) return false;
  if (std::holds_alternative<NullTag>(a)) return std::holds_alternative<NullTag>(b);
  if (std::holds_alternative<NullTag>(b)) return false;
  if (const bool* x = std::get_if<bool>(&a)) {
    const bool* y = std::get_if<bool>(&b);
    return y != nullptr && *x == *y;
  }
  if (const double* x = std::get_if<double>(&a)) {
    const double* y = std::get_if<double>(&b);
    return y != nullptr && *x == *y;
  }
  if (const std::u16string* x = std::get_if<std::u16string>(&a)) {
    const std::u16string* y = std::get_if<std::u16string>(&b);
    return y != nullptr && *x == *y;
  }
  const Array* x = as_array(a);
  return x != nullptr && x == as_array(b);
}

bool equals(const Value& a, const Value& b) {
  const VKind ka = kind_of(a);
  const VKind kb = kind_of(b);
  if (ka == kb) return strict_equals(a, b);

  if ((ka == V_UNDEF && kb == V_NULL) || (ka == V_NULL && kb == V_UNDEF)) return true;

  if (ka == V_BOOL) return equals(Value(to_number(a)), b);
  if (kb == V_BOOL) return equals(a, Value(to_number(b)));

  if (ka == V_NUM && kb == V_STR) return strict_equals(a, Value(to_number(b)));
  if (ka == V_STR && kb == V_NUM) return strict_equals(Value(to_number(a)), b);

  if (ka == V_ARR && (kb == V_NUM || kb == V_STR)) return equals(to_primitive(a), b);
  if (kb == V_ARR && (ka == V_NUM || ka == V_STR)) return equals(a, to_primitive(b));

  return false;
}

}
