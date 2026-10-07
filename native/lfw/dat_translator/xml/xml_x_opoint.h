#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_opoint.ts`：
//   `xml_2_opoint_multi` —— `<multi>`：type/skip_zero 带缺省、min/max 无缺省，
//   尾部删除；`xml_x_opoint_multi` —— 四个属性直写；
//   `xml_x_opoint` —— id/name/kind/oid/pos(软三元)/pos_type，然后 `action` 的
//   下一帧元素**逐个 insert**（不挂 parent 的调用），`dv` 软三元，`multi` 两条**独立 if**
//   （数字写属性、对象插子元素），余下属性一长串；
//   `xml_2_opoint` —— oid 走 `one_or_arr(...) ?? ''`（**缺了给空串**），pos/dv 软三元兜底，
//   `multi` 的兜底链：子元素 → `multi` 属性 → 缺省；尾部删除。
Value xml_2_opoint_multi(const IXMLElement* el);
std::shared_ptr<IXMLElement> xml_x_opoint_multi(IXML& xml, const Value& o,
                                                const std::u16string& tag);
Value xml_2_opoint(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_opoint(IXML& xml, const Value& o, const std::u16string& tag);

}
}
}
