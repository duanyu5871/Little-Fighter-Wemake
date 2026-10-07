#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// Mirrors `src/LFW/loader/check_stage_info.ts`：
//   * TS `errors?: string[]`（可选）⇒ 端口用指针，`nullptr` = 不收集。
//   * `check_phase_info` 的 `stage` / `idx` 在 TS 里**没被用到**（形参照抄）；`errors` 的 TS
//     缺省是 `[]`（丢进临时数组）⇒ 端口 `nullptr` 时写进局部 sink 再丢弃。
bool check_stage_info(const Value& info, std::vector<std::u16string>* errors = nullptr);
bool check_phase_info(const Value& stage, const Value& info, int idx,
                      std::vector<std::u16string>* errors = nullptr);

}
}
