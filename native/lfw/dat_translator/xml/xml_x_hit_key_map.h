#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_hit_key_map.ts`：
//   写：逐键把 `map[key]`（单个或数组，走 `xml_x_t_next_frame`）造元素，出来的每个
//   `set_attr('key', key)`；一个都没有 ⇒ undefined（**不挂 parent**，调用点自己挂）；
//   读：逐 `<tag key=…>` 子元素用 `xml_2_next_frame` 解析进对象，键取 `key` 属性，
//   空对象 ⇒ undefined。
std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_hit_key_map(
    IXML& xml, const Value& map, const std::u16string& tag);
Value xml_2_hit_key_map(const IXMLElement& el, const std::u16string& tag);

}
}
}
