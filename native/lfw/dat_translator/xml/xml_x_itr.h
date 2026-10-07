#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_itr.ts`。
std::shared_ptr<IXMLElement> xml_x_itr(IXML& xml, const Value& i, const std::u16string& tag);
Value xml_2_itr(const IXMLElement& el);

}
}
}
