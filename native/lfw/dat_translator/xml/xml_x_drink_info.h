#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_drink_info.ts`：
//   写：id/name 直写，hp_h/hp_r/mp_h 三组软三元数组（total/value/ticks），
//   假值 ⇒ undefined；
//   读：三组先 `nums_attr_soft`，每个字段 `el.get_num(name, arr?.[i] ?? 缺省)`，
//   尾部 `delete_undefined`（**没有** reorder）。
Value xml_2_drink_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_drink_info(IXML& xml, const Value& d,
                                              const std::u16string& tag);

}
}
}
