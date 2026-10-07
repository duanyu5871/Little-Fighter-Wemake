#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_cpoint.ts`。
Value xml_2_cpoint(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_cpoint(IXML& xml, const Value& i, const std::u16string& tag);

}
}
}
