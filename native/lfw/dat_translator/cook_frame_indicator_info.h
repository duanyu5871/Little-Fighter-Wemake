#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

// 返回 false 表示 TS 那边会抛（`"w" in pic` 对非对象、`?.forEach` 对非数组、给标量挂
// `__indicator_info`）。
bool cook_frame_indicator_info(Value& frame);

}
}
