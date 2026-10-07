#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_map.ts`：
//   `xml_2_map` —— 逐个同 tag 子元素交给 reader，结果的假值/没键（id 或 key）的 跳过，
//   最后空对象给 undefined；
//   `xml_x_map` —— 逐个 map 项（`Object.entries` 序：整数键升序在前）交给 writer，
//   值 nullish（`== void 0`：undefined **和** null）跳过、写不出元素的跳过；缺 id/key 的
//   那两行 `el.get_str("id", key)` 在 TS 里是**死代码**（`get_str(or)` 只读不写），端口照抄；
//   parent 给了就挂上；一个都没有 ⇒ undefined。
using XmlMapReader = std::function<Value(const IXMLElement&)>;

Value xml_2_map(const IXMLElement& el, const std::vector<std::u16string>& tags,
                const XmlMapReader& reader);

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_map(
    IXML& xml, const Value& map, const std::u16string& tag, const XmlElementCreator& writer,
    const std::shared_ptr<IXMLElement>& parent = nullptr);

}
}
}
