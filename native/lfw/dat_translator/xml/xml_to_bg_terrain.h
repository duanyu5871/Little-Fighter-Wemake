#pragma once

#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_to_bg_terrain.ts`：
// 逐字段读（type/x1/x2/z1/z2/h1/h2 带 `terrain_info_new()` 缺省、`name` 无缺省），
// 末尾按字段表重排 —— **没有** `delete_undefined`（TS 就没有）。
Value xml_to_bg_terrain(const IXMLElement& el);

}
}
}
