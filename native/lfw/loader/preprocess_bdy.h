#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// TS `preprocess_bdy({ lfw, data, jobs, frame, bdy })`：
// 端口从 `ctx` 里读 `data` / `frame` / `bdy`，成功时把处理后的 bdy 写回 `ctx.bdy`
// （对应 TS 调用点的 `frame.bdy[i] = preprocess_bdy(ctx)`），失败时返回 false。
// `error` 只在 prefab 解析失败时是 TS `prefab_error(...)` 的 message；
// 其余失败（`bdy` 不是对象、`actions` 不是数组……）在 TS 里是 TypeError，文本不可比。
bool preprocess_bdy(Value& ctx, std::u16string& error);

}
}
