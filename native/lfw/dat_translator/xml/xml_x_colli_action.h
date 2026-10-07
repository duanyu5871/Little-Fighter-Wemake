#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_colli_action.ts`：
// 动作记录 `{ test?, pretest?, type?, data? }`；写时 data 走 `xml.from_object(data, "data")`
// （契约：data 非空 —— TS 那边 undefined 会在 `Object.entries` 上抛 TypeError）。
std::shared_ptr<IXMLElement> xml_x_colli_action(IXML& xml, const Value& action,
                                                const std::u16string& tag);
// 读：`{ test, pretest, type, data }` —— **不做 delete_undefined**（键都留着，值可能是 undefined）。
Value xml_2_colli_action(const IXMLElement& el);

}
}
}
