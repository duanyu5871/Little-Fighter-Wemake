#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_picture_info.ts`：
//   读：id/path 带缺省、`variants` 走 `get_str_arr`（or 被 tool 实现忽略 ⇒ 缺了给 undefined）、
//   row/col/cell_w/cell_h 带缺省；
//   写：`variants?.join()`（undefined 即 `set_attr` 删属性）；
//   另有 map 版两个 `xml_2_map` / `xml_x_map` 薄壳。
Value xml_2_picture_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_picture_info(IXML& xml, const Value& f,
                                                const std::u16string& tag);
Value xml_2_picture_info_map(const IXMLElement& el, const std::u16string& tag);
std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_picture_info_map(
    IXML& xml, const Value& map, const std::u16string& tag);

}
}
}
