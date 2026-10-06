#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {

// Mirrors `src/LFW/defines/IDataInfo.ts`：数据包信息（联机时校验各玩家数据是否一致）。
// TS 里每个字段都是可选的、取值也是 JS 值 ⇒ 端口用 `Value`（没写过的字段就是 `undefined`，
// 写进去的 `null` 也原样留着），这样 `?? 默认值` 的语义与 TS 逐字一致。
//
// 有意不建模：`info` 本身是 nullish 时 TS 读 `info.md5` 会抛 TypeError —— 端口按契约要求 `info` 非空。
struct IDataInfo {
  Value type;
  Value url;
  Value title;
  Value description;
  Value author;
  Value version;
  Value time;
  Value md5;
};

}
