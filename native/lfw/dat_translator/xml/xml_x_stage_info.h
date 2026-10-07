#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_stage_info.ts`：
//   读：id/name/bg/chapter/next/cond_end/act_of_goto_next/is_starting/starting_name/
//   title 带缺省，`phases` 走 `xml_2_non_empty` **缺省给 `[]`**，`group` 走 `get_str_arr`
//   （or 被忽略）；尾部删除 + 重排；
//   写：先 11 个属性（含 `group`），`phases` 子元素挂 el 下；
//   `xml_to_stage_info_list`：tag=="stages" ⇒ 逐个 `<stage>` 子元素、tag=="stage" ⇒ 单个、
//   否则空表（TS 的导出别名 `xml_to_stage_phase_info` 没人用 ⇒ 端口不做）。
Value xml_2_stage_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_stage_info(IXML& xml, const Value& s, const std::u16string& tag);
std::vector<Value> xml_to_stage_info_list(const IXMLElement& el);

}
}
}
