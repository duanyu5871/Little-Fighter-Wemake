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
// `Defines.get_default_keys(player_id)` 的原始返回：TS 是 `map.get(id) || map.get('_')!`，
// 所以可能是**任何值**（甚至是 `undefined`）。取回的是 `default_keys_map` 里那个**共享对象**
// —— `PlayerInfo` 会就地改它（TS 同样如此）。
Value get_default_keys_value(const std::u16string& player_id);
bool is_independent(const std::u16string& team);
bool is_cheat_type(const std::u16string& v);
bool is_difficulty(double v);

}

}
