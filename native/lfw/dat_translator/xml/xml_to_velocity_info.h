#pragma once

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_to_velocity_info.ts`（`out` 省略/undefined ⇒ 新建空对象）：
// 先逐个 `el.get_num(键, out[键])`，再用软属性 `dv/acc/vm/ctrl` 的**数字分量**覆盖
// （`typeof x === 'number'` —— `NaN` 也算数字），最后把值为 undefined 的键删掉。
Value xml_to_velocity_info(const IXMLElement& el, Value out);

}
}
}
