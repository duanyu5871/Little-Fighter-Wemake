#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_from_stage_info.ts`：
// 根 `<stages>`，逐项 `xml_x_stage_info(xml, s, 'stage')`，返回 `root.stringify()`。
std::u16string xml_from_stage_info(IXML& xml, const Value& stages);

}
}
}
