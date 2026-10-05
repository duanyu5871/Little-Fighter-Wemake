#pragma once

#include <string>

namespace lfw {

double string_to_number(const std::u16string& s);
std::u16string number_to_string(double v);
// JS `Number.prototype.toFixed(1)` 的字符串形式：定点一位小数、**尾数 .5 向上取整**
// （不是 to_chars/printf 的 ties-to-even）、`|x| ≥ 1e21` 与 NaN/±Infinity 走 `number_to_string`。
std::u16string number_to_fixed_1(double v);
bool is_str_white_space(char16_t c);

}
