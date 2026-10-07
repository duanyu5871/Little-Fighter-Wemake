#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_difficulty_map.ts`（`DifficultyMap<number>` ⇒ 对象）：
//   写：按 `DifficultyList` 的次序取 map 里**是数字**的项，拼 `k:v` 用 `,` 连接后写进属性；
//   全空/非假值⇒不写（空串也算没得写）；
//   读：`attr` 空串即当作没有，逐段 `k:v` 拆两块 `Number`（`NaN` 跳过；`"1:"` 的 v 是
//   `Number("") === 0`），键用数字字符串，一段都没有 ⇒ undefined。
void xml_x_difficulty_map(const std::shared_ptr<IXMLElement>& el, const std::u16string& attr,
                          const Value& map);
Value xml_2_difficulty_map(const IXMLElement& el, const std::u16string& attr);

}
}
}
