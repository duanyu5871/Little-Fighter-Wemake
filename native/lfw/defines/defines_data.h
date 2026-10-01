#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class Object;

namespace defines {

const std::vector<std::pair<std::u16string, Value>>& table();
const Value* find(const std::u16string& name);
double num(const char16_t* name);

double desire(double ratio);
const Object* get_default_keys(const std::u16string& player_id);
bool is_independent(const std::u16string& team);
bool is_cheat_type(const std::u16string& v);
bool is_difficulty(double v);

}

}
