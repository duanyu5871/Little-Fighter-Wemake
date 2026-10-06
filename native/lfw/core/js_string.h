#pragma once

#include <string>

namespace lfw {

double string_to_number(const std::u16string& s);
std::u16string number_to_string(double v);
// JS `Number.prototype.toFixed(1)` 的字符串形式：定点一位小数、**尾数 .5 向上取整**
// （不是 to_chars/printf 的 ties-to-even）、`|x| ≥ 1e21` 与 NaN/±Infinity 走 `number_to_string`。
std::u16string number_to_fixed_1(double v);
bool is_str_white_space(char16_t c);
// JS `String.prototype.toLowerCase`：Unicode **默认小写映射**（无上下文规则、不合并终结
// sigma、`U+0130` 映射成 `i` + `U+0307`）。覆盖 ASCII / Latin-1 / Latin Extended-A / 希腊 /
// 西里尔 —— 表外码点原样返回（见 README 偏差表）。
std::u16string to_lower_case(const std::u16string& s);

}
