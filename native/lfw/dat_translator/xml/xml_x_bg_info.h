#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_bg_info.ts`：
//   读：bound/zoom/shadowsize 三组软数组作兜底（`??` 链：属性 → 数组项 → 缺省）；
//   `group` 缺省给 `["regular"]`（**字符串数组**）；`height` 缺省直接是
//   `Defines.MODERN_SCREEN_HEIGHT`（不是 ret 的缺省），`delete_undefined` + 重排；
//   写：`group` 是 `b.group?.join() || void 0`（空串等于没写）；bound/shadowsize/zoom 软数组。
Value xml_2_bg_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_bg_info(IXML& xml, const Value& b, const std::u16string& tag);

}
}
}
