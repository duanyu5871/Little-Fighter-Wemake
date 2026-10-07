#pragma once

#include <functional>
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

// Mirrors `src/LFW/dat_translator/xml/xml_x_non_empty.ts`：
//   `xml_2_arr` / `xml_2_non_empty` —— 逐个同名子元素解析成数据（后者空数组给 undefined）；
//   `xml_x_non_empty` —— 逐个数据造元素（creator 给假值就跳过），`parent` 给了就逐个挂上。
// TS 的 parser 还收 `(element, index, array)`，端口的 parser 只收 element（调用点都只用它）；
// 唯一看下标的调用点是 `xml_2_bg_layer(el, index)`（`z` 的兜底用 map 下标）⇒ 另给一组
// 带下标的 `XmlElementParserIdx` 重载。
using XmlElementParser = std::function<Value(const IXMLElement&)>;
using XmlElementParserIdx = std::function<Value(const IXMLElement&, size_t)>;
// creator 对应 TS `(xml, data, tag) => IXMLElement | undefined | null`。
using XmlElementCreator =
    std::function<std::shared_ptr<IXMLElement>(IXML&, const Value&, const std::u16string&)>;

Value xml_2_arr(const IXMLElement& el, const std::u16string& tag,
                const XmlElementParser& parser);
Value xml_2_arr(const IXMLElement& el, const std::u16string& tag,
                const XmlElementParserIdx& parser);
Value xml_2_non_empty(const IXMLElement& el, const std::u16string& tag,
                      const XmlElementParser& parser);
Value xml_2_non_empty(const IXMLElement& el, const std::u16string& tag,
                      const XmlElementParserIdx& parser);

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_non_empty(
    IXML& xml, const Value& arr, const std::u16string& tag, const XmlElementCreator& creator,
    const std::shared_ptr<IXMLElement>& parent = nullptr);

}
}
}
