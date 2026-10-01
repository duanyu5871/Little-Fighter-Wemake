#include "fields.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "core/js_num.h"
#include "core/js_string.h"
#include "core/json.h"

namespace lfw {

namespace {

bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

bool is_null(const Value& v) { return std::holds_alternative<NullTag>(v); }

bool is_object_like(const Value& v) { return is_null(v) || is_array(v) || as_object(v) != nullptr; }

bool is_number(const Value& v) { return std::holds_alternative<double>(v); }

bool is_bool_true(const Value& v) {
  const bool* b = std::get_if<bool>(&v);
  return b != nullptr && *b;
}

bool is_str(const Value& v) { return std::holds_alternative<std::u16string>(v); }

std::u16string as_str(const Value& v) {
  const std::u16string* s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

void assign_all(Object& target, const Value& src) {
  if (const Object* o = as_object(src)) {
    for (const std::u16string& k : o->keys()) {
      const Value* p = o->get(k);
      if (p != nullptr) target.set(k, *p);
    }
    return;
  }
  if (const Array* a = as_array(src)) {
    for (size_t i = 0; i < a->size(); ++i) {
      target.set(number_to_string(static_cast<double>(i)), a->at(i));
    }
    return;
  }
  if (const std::u16string* s = std::get_if<std::u16string>(&src)) {
    for (size_t i = 0; i < s->size(); ++i) {
      target.set(number_to_string(static_cast<double>(i)), Value(std::u16string(1, (*s)[i])));
    }
  }
}

const Value* prop(const Value& o, const char16_t* key) {
  const Object* obj = as_object(o);
  return obj != nullptr ? obj->get(key) : nullptr;
}

const Value* prop(const Value& o, const std::u16string& key) {
  const Object* obj = as_object(o);
  return obj != nullptr ? obj->get(key) : nullptr;
}

bool prop_truthy(const Value& o, const char16_t* key) {
  const Value* v = prop(o, key);
  return v != nullptr && truthy(*v);
}

bool present(const Value& o, const char16_t* key) {
  const Value* v = prop(o, key);
  return v != nullptr && !is_undefined(*v);
}

std::u16string json_text(const Value& v) {
  const std::optional<std::u16string> s = json_stringify(v);
  return s.has_value() ? *s : u"undefined";
}

std::u16string json_join_item(const Value& v) {
  const std::optional<std::u16string> s = json_stringify(v);
  return s.has_value() ? *s : std::u16string();
}

bool is_integer_number(const Value& v) {
  const double* d = std::get_if<double>(&v);
  if (d == nullptr) return false;
  return std::isfinite(*d) && std::floor(*d) == *d;
}

std::u16string str_prop(const Value& o, const char16_t* key) {
  const Value* v = prop(o, key);
  return v != nullptr ? as_str(*v) : std::u16string();
}

void push_error(std::vector<std::u16string>* errors, const std::u16string& msg) {
  if (errors != nullptr) errors->push_back(msg);
}

void push_warning(std::vector<std::u16string>* warnings, const std::u16string& msg) {
  if (warnings != nullptr) warnings->push_back(msg);
}

bool validate_value(const Value& value, const Value& field, const std::u16string& path,
                    std::vector<std::u16string>* errors, std::vector<std::u16string>* warnings);

bool validate_obj(const Value& obj, const Value& field_map, const std::u16string& path,
                  std::vector<std::u16string>* errors, std::vector<std::u16string>* warnings) {
  const Object* map = as_object(field_map);

  bool ok = true;

  for (const std::u16string& key : object_keys(obj)) {
    if (map == nullptr || !map->has(key)) {
      push_warning(warnings, u"[validate_fields] " + path + u" 存在未知字段 \"" + key +
                                 u"\"（未在字段 Map 中声明）");
    }
  }

  if (map == nullptr) return ok;

  for (const std::u16string& key : map->keys()) {
    const Value* field = map->get(key);
    if (field == nullptr) continue;
    const Value* v = prop(obj, key);
    const Value missing;
    if (!validate_value(v != nullptr ? *v : missing, *field, path + u"." + key, errors, warnings)) {
      ok = false;
    }
  }

  return ok;
}

bool validate_value(const Value& value, const Value& field, const std::u16string& path,
                    std::vector<std::u16string>* errors, std::vector<std::u16string>* warnings) {
  const std::u16string type = str_prop(field, u"type");
  const Value* array_v = prop(field, u"array");
  const Value* nullable_v = prop(field, u"nullable");

  if (is_null(value) || is_undefined(value)) {
    if (nullable_v != nullptr && truthy(*nullable_v)) return true;
    push_error(errors, u"[validate_fields] " + path + u" 不允许为空（未声明 nullable），实际为 " +
                           to_string(value));
    return false;
  }

  const bool is_arr = is_array(value);
  const bool array_is_true = array_v != nullptr && is_bool_true(*array_v);
  const bool array_is_auto = array_v != nullptr && is_str(*array_v) && as_str(*array_v) == u"auto";

  if (array_is_true && !is_arr) {
    push_error(errors, u"[validate_fields] " + path + u" 应为数组，实际为 " + json_text(value));
    return false;
  }

  if (is_arr && (array_is_true || array_is_auto)) {
    auto sub_field = std::make_shared<Object>();
    if (const Object* fo = as_object(field)) {
      for (const std::u16string& k : fo->keys()) {
        const Value* p = fo->get(k);
        if (p != nullptr) sub_field->set(k, *p);
      }
    }
    sub_field->set(u"array", Value(false));

    bool ok = true;
    const Array* a = as_array(value);
    for (size_t i = 0; i < a->size(); ++i) {
      const std::u16string item_path =
          path + u"[" + number_to_string(static_cast<double>(i)) + u"]";
      if (!validate_value(a->at(i), Value(sub_field), item_path, errors, warnings)) ok = false;
    }
    return ok;
  }

  const Value* min_v = prop(field, u"min");
  const Value* max_v = prop(field, u"max");

  if (type == u"int") {
    if (!is_number(value) || !is_integer_number(value)) {
      push_error(errors, u"[validate_fields] " + path + u" 应为整数，实际为 " + json_text(value));
      return false;
    }
    if (present(field, u"min") && lt(value, *min_v)) {
      push_error(errors, u"[validate_fields] " + path + u" 不能小于 " + to_string(*min_v) +
                             u"，实际为 " + to_string(value));
      return false;
    }
    if (present(field, u"max") && gt(value, *max_v)) {
      push_error(errors, u"[validate_fields] " + path + u" 不能大于 " + to_string(*max_v) +
                             u"，实际为 " + to_string(value));
      return false;
    }
  } else if (type == u"float" || type == u"flt" || type == u"num") {
    if (!is_number(value)) {
      push_error(errors, u"[validate_fields] " + path + u" 应为数字，实际为 " + json_text(value));
      return false;
    }
    if (present(field, u"min") && lt(value, *min_v)) {
      push_error(errors, u"[validate_fields] " + path + u" 不能小于 " + to_string(*min_v) +
                             u"，实际为 " + to_string(value));
      return false;
    }
    if (present(field, u"max") && gt(value, *max_v)) {
      push_error(errors, u"[validate_fields] " + path + u" 不能大于 " + to_string(*max_v) +
                             u"，实际为 " + to_string(value));
      return false;
    }
  } else if (type == u"string" || type == u"str") {
    const std::u16string* s = std::get_if<std::u16string>(&value);
    if (s == nullptr) {
      push_error(errors, u"[validate_fields] " + path + u" 应为字符串，实际为 " + json_text(value));
      return false;
    }
    const Value* max_len = prop(field, u"maxLength");
    if (present(field, u"maxLength") &&
        gt(Value(static_cast<double>(s->size())), *max_len)) {
      push_error(errors, u"[validate_fields] " + path + u" 长度不能超过 " + to_string(*max_len) +
                             u"，实际为 " + number_to_string(static_cast<double>(s->size())));
      return false;
    }
  } else if (type == u"boolean" || type == u"bool") {
    if (!std::holds_alternative<bool>(value)) {
      push_error(errors, u"[validate_fields] " + path + u" 应为布尔值，实际为 " + json_text(value));
      return false;
    }
  } else if (type == u"object" || type == u"obj") {
    if (std::string_view(type_of(value)) != std::string_view("object") || is_null(value) || is_arr) {
      push_error(errors, u"[validate_fields] " + path + u" 应为对象，实际为 " + json_text(value));
      return false;
    }
    const Value* fields = prop(field, u"fields");
    const Value missing;
    return validate_obj(value, fields != nullptr ? *fields : missing, path, errors, warnings);
  } else if (type == u"map") {
    if (std::string_view(type_of(value)) != std::string_view("object") || is_null(value) || is_arr) {
      push_error(errors,
                 u"[validate_fields] " + path + u" 应为对象（键值映射），实际为 " + json_text(value));
      return false;
    }
    const Value* value_field = prop(field, u"value");
    const Value missing;
    bool ok = true;
    for (const std::u16string& k : object_keys(value)) {
      const Value* v = prop(value, k);
      if (!validate_value(v != nullptr ? *v : missing,
                          value_field != nullptr ? *value_field : missing, path + u"." + k, errors,
                          warnings)) {
        ok = false;
      }
    }
    return ok;
  }

  const Value* options_v = prop(field, u"options");
  if (options_v != nullptr && truthy(*options_v) && !prop_truthy(field, u"bitFlag")) {
    const Array* options = as_array(*options_v);
    bool found = false;
    std::u16string list;
    if (options != nullptr) {
      for (size_t i = 0; i < options->size(); ++i) {
        const Value* ov = prop(options->at(i), u"value");
        if (ov != nullptr && strict_equals(*ov, value)) found = true;
        if (i != 0) list += u", ";
        const Value missing;
        list += json_join_item(ov != nullptr ? *ov : missing);
      }
    }
    if (!found) {
      push_error(errors, u"[validate_fields] " + path + u" 必须为 [" + list + u"] 之一，实际为 " +
                             json_text(value));
      return false;
    }
  }

  return true;
}

}

Value field_desc(const std::u16string& type, const std::vector<Value>& args) {
  auto ret = std::make_shared<Object>();
  ret->set(u"type", Value(type));

  double s = 0;
  for (const Value& v : args) {
    if (const std::u16string* p = std::get_if<std::u16string>(&v)) {
      if (s == 0) {
        ret->set(u"title", Value(*p));
      } else if (s == 1) {
        ret->set(u"desc", Value(*p));
      } else {
        const Value* d = ret->get(u"desc");
        ret->set(u"desc", Value(to_string(d != nullptr ? *d : Value()) + u"\n" + *p));
      }
      s += 1;
    }
    if (is_object_like(v)) assign_all(*ret, v);
  }

  return Value(ret);
}

Value fields_of(const Value& source) {
  auto ret = std::make_shared<Object>();
  auto src = std::make_shared<Object>();
  assign_all(*src, source);

  double order = 0;
  for (const std::u16string& k : src->keys()) {
    const Value* v = src->get(k);
    auto info = std::make_shared<Object>();
    if (v != nullptr) assign_all(*info, *v);
    info->set(u"key", Value(k));
    info->set(u"order", Value(order));
    order += 1;
    ret->set(k, Value(info));
  }

  return Value(ret);
}

Value fields_map_2_fields_obj(const Value& field_map) {
  auto ret = std::make_shared<Object>();
  const Object* map = as_object(field_map);
  if (map == nullptr) return Value(ret);

  for (const std::u16string& k : map->keys()) {
    const Value* v = map->get(k);
    if (v != nullptr) ret->set(k, *v);
  }

  return Value(ret);
}

void reorder_fields(Value& obj, const Value& field_map) {
  Object* o = as_object(obj);
  if (o == nullptr) return;

  const std::vector<std::u16string> all_keys = o->keys();

  std::vector<std::u16string> known;
  for (const std::u16string& k : all_keys) {
    const Value* f = prop(field_map, k);
    const Value* order = f != nullptr ? prop(*f, u"order") : nullptr;
    if (order != nullptr && !is_undefined(*order)) known.push_back(k);
  }

  std::stable_sort(known.begin(), known.end(),
                   [&](const std::u16string& a, const std::u16string& b) {
                     const Value* fa = prop(field_map, a);
                     const Value* fb = prop(field_map, b);
                     const Value* oa = fa != nullptr ? prop(*fa, u"order") : nullptr;
                     const Value* ob = fb != nullptr ? prop(*fb, u"order") : nullptr;
                     const double na = oa != nullptr && !is_null(*oa) ? to_number(*oa) : 0;
                     const double nb = ob != nullptr && !is_null(*ob) ? to_number(*ob) : 0;
                     return na - nb < 0;
                   });

  std::vector<std::pair<std::u16string, Value>> kvs;
  for (const std::u16string& k : known) {
    const Value* v = o->get(k);
    kvs.emplace_back(k, v != nullptr ? *v : Value());
  }
  for (const std::u16string& k : all_keys) {
    if (std::find(known.begin(), known.end(), k) != known.end()) continue;
    const Value* v = o->get(k);
    kvs.emplace_back(k, v != nullptr ? *v : Value());
  }

  for (const std::u16string& k : all_keys) o->remove(k);
  for (std::pair<std::u16string, Value>& kv : kvs) o->set(kv.first, std::move(kv.second));
}

Value to_array(const Value& v) {
  if (is_null(v) || is_undefined(v)) return Value(std::make_shared<Array>());
  if (is_array(v)) return v;
  auto arr = std::make_shared<Array>();
  arr->push_back(v);
  return Value(arr);
}

bool validate_fields(const Value& obj, const Value& field_map,
                     std::vector<std::u16string>* errors, std::vector<std::u16string>* warnings) {
  if (std::string_view(type_of(obj)) != std::string_view("object") || is_null(obj) || is_array(obj)) {
    push_error(errors, u"[validate_fields] 根值应为对象，实际为 " + json_text(obj));
    return false;
  }
  return validate_obj(obj, field_map, u"root", errors, warnings);
}

}
