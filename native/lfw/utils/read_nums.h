#pragma once

#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// TS `ui/utils/read_nums.ts`：把 `string | number[]` 读成固定长度的数字数组。
// `ui/` 还没移植、而它是纯函数（`loader/preprocess_frame` 要用），先放在 `utils/` 下。
//
// 返回 false ⇒ TS 会 throw（`fallbacks` 不是数字数组、`src` 既不是字符串也不是数字数组）；
// 此时若给定了 `error`，写入与 TS 一致的 `[read_nums] …` 文本。
// 成功时 `out` 的长度恒为 `len`（`len < 1` 时为空），元素可能是 `undefined`
// （TS 里 `src[idx]` 越界读成 `undefined`）或 `NaN`。
bool read_nums(const Value& src, double len, const Value& fallbacks, std::vector<Value>& out,
               std::u16string* error = nullptr);

}
