#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_partial_world_dataset.ts`：
//   `xml_2_partial_world_dataset` —— `el?.as_object()`（`nullptr` 当 TS 的 undefined）；
//   `xml_x_partial_world_dataset` —— 假值或**空对象**都给 undefined，否则
//   `xml.from_object(i, tag)`。
Value xml_2_partial_world_dataset(const IXMLElement* el);
std::shared_ptr<IXMLElement> xml_x_partial_world_dataset(IXML& xml, const Value& i,
                                                         const std::u16string& tag);

}
}
}
