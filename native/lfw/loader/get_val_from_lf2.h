#pragma once

#include <string>

#include "lfw/base/expression.h"

namespace lfw {

namespace loader {

// `src/LFW/loader/get_val_from_lf2.ts`：一个只有 `default` 分支的 switch，恒 `undefined`，
// 所以这里也恒 `nullptr`。
//
// TS 的签名是 `IValGetterGetter<LFW>`，但 `LFW` 还没有移植（DESIGN §61.3），所以上下文
// 类型先留成模板参数 —— 反正无论上下文是什么，返回的都是空指针，语义不会变。
template <typename Ctx>
ValGetter<Ctx> get_val_from_lf2(const std::u16string& word) {
  (void)word;
  return nullptr;
}

}

}
