#pragma once

#include <string>

#include "lfw/base/expression.h"

namespace lfw {

namespace stage {
class Stage;
}  // namespace stage

namespace loader {

// TS `loader/get_val_getter_from_stage.ts`：固定表（`S_Val` 的 12 个词）+ 未命中时向
// `get_val_from_world` 回落（那层是死分支 ⇒ 恒 `nullptr`）。
// `stage_world_val_getters` 备忘录在 TS 里没有消费者 ⇒ 不建形（记 DESIGN）。
ValGetter<stage::Stage> get_val_getter_from_stage(const std::u16string& word);

}  // namespace loader
}  // namespace lfw
