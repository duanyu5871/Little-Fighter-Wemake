#include "lfw/dat_translator/helpers.h"

#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/core/json.h"
#include "lfw/core/json5.h"
#include "lfw/core/value.h"
#include "lfw/defines/labels.h"
#include "lfw/utils/type_check.h"

namespace lfw {
namespace dat_translator {
namespace {

double half_away_scaled(double a, double scale) {
  const double p = a * scale;
  const double err = std::fma(a, scale, -p);
  double k = std::floor(p);
  const double frac = p - k;
  if (frac > 0.5) {
    k += 1;
  } else if (frac == 0.5) {
    if (!(err < 0)) k += 1;
  }
  return k;
}

Value json_round_trip(const Value& src) {
  const std::optional<std::u16string> text = json_stringify(src);
  if (!text.has_value()) return Value();
  const std::optional<Value> parsed = json_parse(*text);
  return parsed.has_value() ? *parsed : Value();
}

Value json5_round_trip(const Value& src) {
  const Json5TextResult text = json5_stringify(src);
  if (!text.ok) return Value();
  const Json5Result parsed = json5_parse(text.text);
  return parsed.ok ? parsed.value : Value();
}

Value merge_copy(const Value& src, bool json5, const Object& edit) {
  const Value parsed = json5 ? json5_round_trip(src) : json_round_trip(src);
  Object out;
  if (const Object* po = as_object(parsed)) {
    for (const std::u16string& k : po->keys()) {
      const Value* v = po->get(k);
      if (v != nullptr) out.set(k, *v);
    }
  }
  for (const std::u16string& k : edit.keys()) {
    const Value* v = edit.get(k);
    if (v != nullptr) out.set(k, *v);
  }
  return Value(std::make_shared<Object>(out));
}

std::optional<double> take_num_impl(Object& any, const std::u16string& key, bool positive, bool not_zero,
                                    std::function<double(double)> fn) {
  const Value v = Value(to_number(take(any, key)));
  if (!is_num(v)) return std::nullopt;
  if (positive && !is_positive(v)) return std::nullopt;
  if (not_zero && !not_zero_num(v)) return std::nullopt;
  const double n = std::get<double>(v);
  if (n == -842150451) return std::nullopt;
  return fn ? std::optional<double>(fn(n)) : std::optional<double>(n);
}

}

Value take(Object& any, const std::u16string& key) {
  const Value* v = any.get(key);
  Value ret = v != nullptr ? *v : Value();
  any.remove(key);
  return ret;
}

std::optional<std::u16string> take_str(Object& any, const std::u16string& key) {
  const Value* v = any.get(key);
  if (v == nullptr) return std::nullopt;
  const std::u16string* s = std::get_if<std::u16string>(v);
  if (s == nullptr) return std::nullopt;
  const std::u16string ret = *s;
  any.remove(key);
  return ret;
}

std::optional<double> take_num(Object& any, const std::u16string& key,
                               std::function<double(double)> fn) {
  return take_num_impl(any, key, false, false, std::move(fn));
}

std::optional<double> take_positive_num(Object& any, const std::u16string& key,
                                        std::function<double(double)> fn) {
  return take_num_impl(any, key, true, false, std::move(fn));
}

std::optional<double> take_not_zero_num(Object& any, const std::u16string& key,
                                        std::function<double(double)> fn) {
  return take_num_impl(any, key, false, true, std::move(fn));
}

std::pair<double, double> take_raw_frame_mp(Object& frame) {
  const Value raw = take(frame, u"mp");
  if (!is_num(raw)) return {0, 0};
  const double mp = std::get<double>(raw);
  if (mp < 1000 && mp > -1000) return {mp, 0};
  const double rem = std::fmod(mp, 1000);
  const double hp = (mp - rem) / 100;
  return {rem, hp};
}

Value take_number(Object& any, const std::u16string& key, const Value& or_value) {
  const Value* v = any.get(key);
  const bool is_number = v != nullptr && std::holds_alternative<double>(*v);
  Value ret = is_number ? *v : or_value;
  any.remove(key);
  return ret;
}

Object& set_hit_flag(Object& info, const Value& value) {
  info.set(u"hit_flag", value);
  info.set(u"hit_flag_name", Value(defines::get_hit_flag_name(value)));
  return info;
}

Object& set_bdy_kind(Object& bdy, const Value& kind) {
  bdy.set(u"kind", kind);
  bdy.set(u"kind_name", Value(defines::bdy_kind_full_name(kind)));
  return bdy;
}

Value& delete_undefined(Value& o) {
  Object* p = as_object(o);
  if (p == nullptr) return o;
  const std::vector<std::u16string> keys = p->keys();
  for (const std::u16string& k : keys) {
    const Value* v = p->get(k);
    if (v != nullptr && std::string(type_of(*v)) == "undefined") p->remove(k);
  }
  return o;
}

double fixed_float(double n, double digits) {
  if (!std::isfinite(n)) return n;
  if (std::abs(n) >= 1e21) return n;
  double d = std::trunc(digits);
  if (!(d >= 0)) d = 0;
  if (d > 100) d = 100;
  const double scale = std::pow(10.0, d);
  const double a = std::abs(n);
  const double k = half_away_scaled(a, scale);
  const double out = k / scale;
  return n < 0 ? -out : out;
}

std::pair<bool, std::u16string> find_float(const Value& v, const std::u16string& path) {
  if (std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v)) {
    return {false, path};
  }
  if (const double* d = std::get_if<double>(&v)) return {!is_int(*d), path};
  if (const std::shared_ptr<Array>* a = std::get_if<std::shared_ptr<Array>>(&v)) {
    if (*a != nullptr) {
      for (size_t k = 0; k < (*a)->size(); ++k) {
        const std::pair<bool, std::u16string> r =
            find_float((*a)->at(k), path + u"/" + number_to_string(static_cast<double>(k)));
        if (r.first) return r;
      }
    }
    return {false, path};
  }
  if (const std::shared_ptr<Object>* o = std::get_if<std::shared_ptr<Object>>(&v)) {
    if (*o != nullptr) {
      for (const std::u16string& k : (*o)->keys()) {
        const Value* item = (*o)->get(k);
        if (item == nullptr) continue;
        const std::pair<bool, std::u16string> r = find_float(*item, path + u"/" + k);
        if (r.first) return r;
      }
    }
    return {false, path};
  }
  return {false, path};
}

Value copy_bdy_info(const Value& src, const Object& edit) { return merge_copy(src, false, edit); }

Value copy_itr_info(const Value& src, const Object& edit) { return merge_copy(src, true, edit); }

}

}
