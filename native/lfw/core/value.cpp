#include "value.h"

#include <cmath>
#include <limits>

#include "js_string.h"

namespace lfw {

namespace {

enum VKind { V_UNDEF, V_NULL, V_BOOL, V_NUM, V_STR, V_ARR, V_OBJ };

VKind kind_of(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return V_UNDEF;
  if (std::holds_alternative<NullTag>(v)) return V_NULL;
  if (std::holds_alternative<bool>(v)) return V_BOOL;
  if (std::holds_alternative<double>(v)) return V_NUM;
  if (std::holds_alternative<std::u16string>(v)) return V_STR;
  if (std::holds_alternative<std::shared_ptr<Object>>(v)) return V_OBJ;
  return V_ARR;
}

bool is_array_index(const std::u16string& k, uint32_t& out) {
  if (k.empty() || k.size() > 10) return false;
  if (k.size() > 1 && k[0] == u'0') return false;
  uint64_t v = 0;
  for (char16_t c : k) {
    if (c < u'0' || c > u'9') return false;
    v = v * 10 + static_cast<uint64_t>(c - u'0');
  }
  if (v > 4294967294ull) return false;
  out = static_cast<uint32_t>(v);
  return true;
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

const Object* as_object(const Value& v) {
  const std::shared_ptr<Object>* p = std::get_if<std::shared_ptr<Object>>(&v);
  return p && *p ? p->get() : nullptr;
}

Object* as_object(Value& v) {
  std::shared_ptr<Object>* p = std::get_if<std::shared_ptr<Object>>(&v);
  return p && *p ? p->get() : nullptr;
}

bool Object::has(const std::u16string& key) const { return get(key) != nullptr; }

const Value* Object::get(const std::u16string& key) const {
  uint32_t idx = 0;
  if (is_array_index(key, idx)) {
    const auto it = _ints.find(idx);
    return it == _ints.end() ? nullptr : &it->second;
  }
  for (const auto& kv : _strs) {
    if (kv.first == key) return &kv.second;
  }
  return nullptr;
}

void Object::set(const std::u16string& key, Value v) {
  uint32_t idx = 0;
  if (is_array_index(key, idx)) {
    _ints[idx] = std::move(v);
    return;
  }
  for (auto& kv : _strs) {
    if (kv.first == key) {
      kv.second = std::move(v);
      return;
    }
  }
  _strs.emplace_back(key, std::move(v));
}

bool Object::remove(const std::u16string& key) {
  uint32_t idx = 0;
  if (is_array_index(key, idx)) return _ints.erase(idx) != 0;
  for (size_t i = 0; i < _strs.size(); ++i) {
    if (_strs[i].first == key) {
      _strs.erase(_strs.begin() + static_cast<std::ptrdiff_t>(i));
      return true;
    }
  }
  return false;
}

std::vector<std::u16string> Object::keys() const {
  std::vector<std::u16string> out;
  out.reserve(size());
  for (const auto& kv : _ints) out.push_back(number_to_string(static_cast<double>(kv.first)));
  for (const auto& kv : _strs) out.push_back(kv.first);
  return out;
}

std::vector<std::u16string> object_keys(const Value& v) {
  const Object* o = as_object(v);
  return o != nullptr ? o->keys() : std::vector<std::u16string>();
}

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
  if (const Array* a = as_array(v)) return array_join(*a);
  return u"[object Object]";
}

Value to_primitive(const Value& v) {
  if (const Array* a = as_array(v)) return Value(array_join(*a));
  if (as_object(v) != nullptr) return Value(std::u16string(u"[object Object]"));
  return v;
}

double to_number(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return std::numeric_limits<double>::quiet_NaN();
  if (std::holds_alternative<NullTag>(v)) return 0.0;
  if (const bool* b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
  if (const double* d = std::get_if<double>(&v)) return *d;
  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return string_to_number(*s);
  if (const Array* a = as_array(v)) return string_to_number(array_join(*a));
  return string_to_number(u"[object Object]");
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
  if (const Array* xa = as_array(a)) return xa == as_array(b);
  if (const Object* xo = as_object(a)) return xo == as_object(b);
  return false;
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

  if ((ka == V_ARR || ka == V_OBJ) && (kb == V_NUM || kb == V_STR)) return equals(to_primitive(a), b);
  if ((kb == V_ARR || kb == V_OBJ) && (ka == V_NUM || ka == V_STR)) return equals(a, to_primitive(b));

  return false;
}

std::optional<bool> less_than(const Value& a, const Value& b) {
  const Value px = to_primitive(a);
  const Value py = to_primitive(b);
  const std::u16string* sx = std::get_if<std::u16string>(&px);
  const std::u16string* sy = std::get_if<std::u16string>(&py);
  if (sx != nullptr && sy != nullptr) return *sx < *sy;
  const double nx = to_number(px);
  const double ny = to_number(py);
  if (std::isnan(nx) || std::isnan(ny)) return std::nullopt;
  return nx < ny;
}

bool lt(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && *r;
}

bool gt(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(b, a);
  return r.has_value() && *r;
}

bool le(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(b, a);
  return r.has_value() && !*r;
}

bool ge(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && !*r;
}

}
