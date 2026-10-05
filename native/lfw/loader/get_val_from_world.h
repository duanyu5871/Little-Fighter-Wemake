#pragma once

#include <string>

#include "lfw/base/expression.h"
#include "lfw/loader/get_val_from_lf2.h"

namespace lfw {

namespace loader {

// `src/LFW/loader/get_val_from_world.ts`：查 `WorldVal` 表（现在还没有任何一项，
// 只剩 `default`），未命中时向 `get_val_from_lf2` 回落 —— 那层是死分支，所以恒 `nullptr`。
//
// TS 的回落还会包一层投影：`(world, ...arg) => fallback(world.lfw, ...arg)`。端口里的
// `ValGetter` 是裸函数指针（`base/expression.h`），捕获不了 `world → world.lfw` 这一步，
// 等 `LF2Val` 有实现时需要一个「按 getter 生成的静态适配器」（DESIGN §61.4），此处不预造。
template <typename Ctx>
ValGetter<Ctx> get_val_from_world(const std::u16string& word) {
  return get_val_from_lf2<Ctx>(word);
}

}

}
