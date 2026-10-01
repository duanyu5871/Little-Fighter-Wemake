#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

inline constexpr const char16_t* kFieldTodo = u"";
inline constexpr const char16_t* kFieldString = u"string";
inline constexpr const char16_t* kFieldFloat = u"float";
inline constexpr const char16_t* kFieldInt = u"int";
inline constexpr const char16_t* kFieldBool = u"boolean";
inline constexpr const char16_t* kFieldObject = u"object";
inline constexpr const char16_t* kFieldMap = u"map";

Value field_desc(const std::u16string& type, const std::vector<Value>& args);
Value fields_of(const Value& source);
Value fields_map_2_fields_obj(const Value& field_map);
void reorder_fields(Value& obj, const Value& field_map);
Value to_array(const Value& v);

bool validate_fields(const Value& obj, const Value& field_map,
                     std::vector<std::u16string>* errors, std::vector<std::u16string>* warnings);

}
