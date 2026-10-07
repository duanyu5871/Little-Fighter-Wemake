#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_dat_index.ts`：
//   `xml_x_dat_index` —— 按固定次序写 8 个属性（tag 缺省 `dat_index`）；
//   `xml_2_dat_index` —— id/type/file 带 `dat_index_new()` 缺省、hash/alias/skipped/bot 无缺省，
//   `groups` 走 `strs_attr`（缺属性给 undefined），最后 `delete_undefined` + 按字段表重排。
std::shared_ptr<IXMLElement> xml_x_dat_index(IXML& xml, const Value& idx,
                                             const std::u16string& tag = u"dat_index");
Value xml_2_dat_index(const IXMLElement& el);

}
}
}
