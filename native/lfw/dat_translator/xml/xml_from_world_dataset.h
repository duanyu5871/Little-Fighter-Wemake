#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_from_world_dataset.ts`（tag 缺省 `dataset`）：
// 按 `world_dataset_fields` 的表序把**非 `undefined`**（null 也算有）的字段凑成对象，
// 一个都没有 ⇒ undefined，否则 `xml.from_object(item, tag)`。
std::shared_ptr<IXMLElement> xml_from_world_dataset(IXML& xml, const Value& data,
                                                    const std::u16string& tag = u"dataset");

}
}
}
