#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_from_data_lists.ts`：
// 根 `<data>`，五组按固定次序 [obj←objects, background←backgrounds, stages←stages,
// bot←bots, moves←moves]；空组跳过，每项一个 `xml_x_dat_index(xml, item, tag)`。
std::shared_ptr<IXMLElement> xml_from_data_lists(IXML& xml, const Value& lists);

}
}
}
