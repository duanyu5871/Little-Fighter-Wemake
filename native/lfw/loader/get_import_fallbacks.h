#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// TS `get_import_fallbacks(name)`：输入引入名，输出（备选名列表, 命中的后缀）。
// `name` 不是字符串时 TS 在 `path.endsWith(...)` 上抛 ⇒ 端口返回 false。
// 备选名里**包含**原名（`fallbacks[0]` 之后的顺序见 .cpp 的注释）。
bool get_import_fallbacks(const Value& name, std::vector<std::u16string>& fallbacks,
                          std::u16string& suffix);

}
}
