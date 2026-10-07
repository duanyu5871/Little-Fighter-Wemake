#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_frame_indexes.ts`：
//   写：25 个属性；成对的 `falling/bouncing/critical_hit/grand_injured/lying` 取
//   `索引?.[1]` 与 `索引?.[-1]`（**字符串键 "-1"**）；假值整体 ⇒ undefined；
//   读：`nullptr` 当 TS 的 undefined；in_the_skys/throwings/on_hands 走 `get_str_arr`（or
//   被忽略），`injured`/`lying` 一对是 **`get_str`**（单个字符串，不是数组！）—— 与写向
//   不对称；成对子对象 `{[1]: a, [-1]: b}` 两项都真值才给；删除 + 按字段表重排。
std::shared_ptr<IXMLElement> xml_x_frame_indexes(IXML& xml, const Value& indexes,
                                                 const std::u16string& tag);
Value xml_2_frame_indexes(const IXMLElement* el);

}
}
}
