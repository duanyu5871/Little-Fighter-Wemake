#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_stage_object_info.ts`：
//   写：一长串属性；hp/mp 后各跟一个 `xml_x_difficulty_map`（同名属性 `hp`/`mp`）；
//   读：`id` 走 `get_str_arr`（or 被忽略 ⇒ 缺了给 undefined）、其余带缺省，
//   `hp_map`/`mp_map` 走 `xml_2_difficulty_map`，`delete_undefined` + 按字段表重排。
Value xml_2_stage_object_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_stage_object_info(IXML& xml, const Value& o,
                                                     const std::u16string& tag);

}
}
}
