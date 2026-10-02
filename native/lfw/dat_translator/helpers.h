#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class Object;

namespace dat_translator {

Value take(Object& any, const std::u16string& key);
std::optional<std::u16string> take_str(Object& any, const std::u16string& key);
std::optional<double> take_num(Object& any, const std::u16string& key,
                               std::function<double(double)> fn = nullptr);
std::optional<double> take_positive_num(Object& any, const std::u16string& key,
                                        std::function<double(double)> fn = nullptr);
std::optional<double> take_not_zero_num(Object& any, const std::u16string& key,
                                        std::function<double(double)> fn = nullptr);
std::pair<double, double> take_raw_frame_mp(Object& frame);
Value take_number(Object& any, const std::u16string& key, const Value& or_value);

Object& set_hit_flag(Object& info, const Value& value);
Object& set_bdy_kind(Object& bdy, const Value& kind);
std::pair<Value, Value> hit_flag_pair(const Value& value);
Object& edit_info(Object& src, const std::vector<Value>& edits);

double fixed_float(double n, double digits = 1);
std::pair<bool, std::u16string> find_float(const Value& v, const std::u16string& path = std::u16string());

Value copy_bdy_info(const Value& src, const Object& edit);
Value copy_itr_info(const Value& src, const Object& edit);

Value& delete_undefined(Value& o);

}

}
