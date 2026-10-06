#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// TS `preprocess_entity_data(ctx)`（`async`）：端口从 `ctx` 读 `data` / `lfw` / `jobs` / `errors`，
// 成功时就地改 `ctx.data`（TS 返回同一个 `data` 对象），失败返回 false。
//
// `error` 只在 prefab 解析失败时是 TS `prefab_error(...)` 的 message；其余失败（`ctx` / `data` /
// `lfw` / `data.base` / `errors` 缺失、`jobs` 上挂不上任务、`?.forEach` 不是数组……）在 TS 里是
// TypeError，文本不可比。加载任务与末尾的 `data.xml` 端口不落地，见 DESIGN §66。
bool preprocess_entity_data(Value& ctx, std::u16string& error);

}
}
