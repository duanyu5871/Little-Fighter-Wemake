#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_stage_phase_info.ts`：
//   `xml_x_sound_play_info` —— ⚠ **TS 的怪癖照抄**：`if (!s || s.path.trim()) return;`
//   —— 只有「缺省或 path 为空串」才写得出来，path 有内容反而返回 undefined；
//   `xml_2_sound_play_info` —— 五个字段全带缺省；
//   `xml_x_stage_phase_info` —— 一串属性 + 五个难度映射 + sounds/objects/dialogs 三组
//   子元素（挂在 el 下）；
//   `xml_2_stage_phase_info` —— 属性 + 难度映射读回，然后三组 `xml_2_non_empty` 的调用
//   **结果被丢弃**（TS 写的就是裸调用，sounds/objects/dialogs 不进结果），尾部删除 + 重排。
std::shared_ptr<IXMLElement> xml_x_sound_play_info(IXML& xml, const Value& s,
                                                   const std::u16string& tag);
Value xml_2_sound_play_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_stage_phase_info(IXML& xml, const Value& p,
                                                    const std::u16string& tag);
Value xml_2_stage_phase_info(const IXMLElement& el);

}
}
}
