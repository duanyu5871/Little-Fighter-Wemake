#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// TS `preprocess_itr({ lfw, data, jobs, itr })`：
// 端口从 `ctx` 里读 `data` / `itr`，成功时把处理后的 itr 写回 `ctx.itr`，失败时返回 false。
// `error` 只在 prefab 解析失败时是 TS `prefab_error(...)` 的 message。
bool preprocess_itr(Value& ctx, std::u16string& error);

}
}
