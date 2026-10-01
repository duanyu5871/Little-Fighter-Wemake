#pragma once

#include <map>
#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace defines {

Value bdy_kind_name(const Value& v);
std::u16string bdy_kind_full_name(const Value& v);
Value wpoint_kind_name(const Value& v);
std::u16string wpoint_kind_full_name(const Value& v);

Value js_enum_get(const char16_t* enum_name, const Value& v);

std::u16string get_hit_flag_name(const Value& v);
std::u16string get_hit_flag_full_name(const Value& v);
std::u16string get_hit_flag_desc(const Value& v);

std::map<std::u16string, std::u16string> hit_flag_name_map();

}

}
