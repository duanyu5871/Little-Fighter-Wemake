#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_to_world_dataset.ts`：
// 逐 `<dataset>` 子元素取 `name` 属性，在 `world_dataset_fields` 里查字段；
// `int`/`float` 走 `child.as_number()` —— ⚠ tool 实现里 `as_number` 是**死值**
// （类型门禁在 `as_string` 里，数字子元素过不去）⇒ 这类字段恒 `undefined`，照抄；
// `string` 走 `as_string()`、`boolean` 走 `as_value()`；`nullptr` ⇒ 空对象（TS 的 `{}`）。
Value xml_to_world_dataset(const IXMLElement* el);

}
}
}
