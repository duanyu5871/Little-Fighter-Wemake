#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_bg_data.ts`（tag 缺省 `background`）：
//   读：id/alias_id，`type` 恒 `"background"`，`base` 走 `merge_by_tag`（bg_info）、
//   `dataset` 走 `merge_by_tag`（partial_world_dataset）、`layers` 走 `xml_2_non_empty`
//   （`xml_2_bg_layer` 要下标）、`terrain` 走 `xml_2_non_empty`（xml_to_bg_terrain）；
//   尾部删除 + 重排；
//   写：id、插入 base、插入 dataset、layers 逐个挂 —— ⚠ **不写 terrain**（TS 就没写）。
Value xml_2_bg_data(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_bg_data(IXML& xml, const Value& data,
                                           const std::u16string& tag = u"background");

}
}
}
