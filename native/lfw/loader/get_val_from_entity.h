#pragma once

#include <string>
#include <utility>
#include <vector>

#include "lfw/base/expression.h"
#include "lfw/entity/entity.h"

namespace lfw {

namespace loader {

// `src/LFW/loader/get_val_from_entity.ts` 里的 `entity_val_getters` 表，声明顺序照抄。
// 键是 `defines/entity_val.h` 的 `entity_val::k*`（即 `E_Val` 成员的字符串值）。
const std::vector<std::pair<std::u16string, ValGetter<Entity>>>& entity_val_getters();

// TS 的 `get_val_getter_from_entity`：先查上表，未命中才向 `get_val_from_world` 回落。
// 那个回落现在是死分支（`get_val_from_world.h`），所以未命中一律给出 `nullptr`
// （对应 TS 的 `undefined`）。
ValGetter<Entity> get_val_getter_from_entity(const std::u16string& word);

}

}
