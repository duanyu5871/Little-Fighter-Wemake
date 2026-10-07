#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_dialog_info.ts`：
//   读：type/fighter/pause(布尔)/i18n/close_by/hide_stats/end_test 全带缺省，
//   `delete_undefined` + 按字段表重排；
//   写：假值（undefined/null）⇒ undefined，否则逐属性 `set_attr`（undefined 属性自动删）。
Value xml_2_dialog_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_dialog_info(IXML& xml, const Value& d,
                                               const std::u16string& tag);

}
}
}
