#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_bg_layer.ts`：
//   读：`(el, index)` —— pos/size/rect/offsetAnim 四组硬数组作 `??` 兜底链，
//   `z` 的兜底链是 属性 → **index** → 缺省（`xml_2_non_empty` 走 `Array.map` 会带下标，
//   见 `xml_x_non_empty.h` 的 `XmlElementParserIdx`）；
//   写：22 个属性按固定次序（width…opacity），`color`/`id`/`name`/`file` 在靠后位置。
Value xml_2_bg_layer(const IXMLElement& el, size_t index);
std::shared_ptr<IXMLElement> xml_x_bg_layer(IXML& xml, const Value& l,
                                            const std::u16string& tag);

}
}
}
