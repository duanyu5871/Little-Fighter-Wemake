#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_armor_info.ts`（`a` 假值 ⇒ 不造、回 nullptr）。
std::shared_ptr<IXMLElement> xml_x_armor_info(IXML& xml, const Value& a, const std::u16string& tag);
// `el` 为空 ⇒ undefined。
Value xml_2_armor_info(const IXMLElement* el);

}
}
}
