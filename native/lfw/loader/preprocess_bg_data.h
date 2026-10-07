#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

// `warnings` / `errors` 是 TS 里 `Ditto.warn(...)` / `Ditto.error(...)` 的宿主缝：TS 传的是
// `SV.Default.warnings` / `.errors` **整个数组**（一次一个参数）⇒ 端口把消息追加进 sink。
Value preprocess_bg_data(Value& data, std::vector<std::u16string>* warnings = nullptr,
                         std::vector<std::u16string>* errors = nullptr);

}
}
