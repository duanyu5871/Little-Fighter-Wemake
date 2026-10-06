#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// TS `preprocess_frame({ lfw, data, jobs, frame })`：
// 端口从 `ctx` 读 `data` / `frame`，成功时把处理后的 frame 写回 `ctx.frame`
// （对应 TS 调用点的 `o[fid] = preprocess_frame({ ...ctx, frame })`），失败返回 false。
// `error` 只在 prefab 解析失败时是 TS `prefab_error(...)` 的 message。
bool preprocess_frame(Value& ctx, std::u16string& error);

}
}
