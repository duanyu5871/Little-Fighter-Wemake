#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/one_or_arr.ts`：
//   `one_or_arr(mess)` —— 不是数组原样返回；数组只 1 项给那一项；否则给数组本身；
//   `non_empty(mess)` —— 是数组且非空才给数组，否则 `undefined`。
Value one_or_arr(const Value& mess);
Value non_empty(const Value& mess);

// 便捷重载：`el.get_str_arr(name)` 的返回值（`string[] | undefined`）直接喂。
Value one_or_arr(const std::optional<std::vector<std::u16string>>& mess);
Value non_empty(const std::optional<std::vector<std::u16string>>& mess);

}
}
}
