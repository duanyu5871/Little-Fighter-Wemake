#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_chase.ts`（含 `overshoot` 的三种写法：
// 单数字 = 三轴同值；逗号分隔按位置，缺的分量不写；全缺省 ⇒ 不写属性）。
Value xml_2_chase(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_chase(IXML& xml, const Value& c, const std::u16string& tag);

}
}
}
