#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_wpoint.ts`。
// ⚠ `xml_2_wpoint` **不做** delete_undefined：默认对象里没有 `dvx/dvy/dvz`，
// 缺值时这三键会以 undefined 留在结果里（`xml_x_wpoint` 的 `set_arr_attr_soft("v", …)` 又把它删掉）。
Value xml_2_wpoint(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_wpoint(IXML& xml, const Value& i, const std::u16string& tag);

}
}
}
