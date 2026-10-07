#pragma once

#include <functional>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/merge_by_tag.ts`：
// 取同名子元素逐个解析（parser 给同一棵树里的**不同**子元素），第 1 个的结果作底、
// 后续结果按键 `Object.assign` 逐层合并（后者覆盖前者、新键按来源顺序追加）；
// `target` 真值（对象）时把**合并结果**再 `Object.assign` 进它（就地改）。
// 没有同名子元素 ⇒ `undefined`（`target` 不动）。契约：parser 给对象。
Value merge_by_tag(const IXMLElement& el, const std::u16string& tag,
                   const std::function<Value(const IXMLElement&)>& parser);
Value merge_by_tag(const IXMLElement& el, const std::u16string& tag,
                   const std::function<Value(const IXMLElement&)>& parser, Value target);

}
}
}
