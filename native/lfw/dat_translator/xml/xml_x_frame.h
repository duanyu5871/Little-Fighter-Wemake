#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_frame.ts`（帧信息，本层最大的两个函数）：
//   写：id/name/ref，`pic`/`pics` 逐个造 `<pic>`，`model` 走 `xml_x_frame_model`，
//   state/wait，`next` 下一帧串；`center`/`size` 是 `[a, b].join()`（缺值就是 `","`）；
//   `sound` 单值/数组两态（数组项直接 `set_attr("value", …)`，假值即删属性）；
//   hp/mp、一串布尔位、hit/hold/key_down/key_up 四组击键映射、bdy/itr/opoint 三串
//   （`?.map(...).forEach(insert)`）、bpoint/wpoint/cpoint/chase；最后按
//   `FRAME_BEHAVIOR_LABEL_MAP` / `StateEnumNames` 补 `behavior_label` / `state_label`
//   （`f.behavior != void 0` 且映射值真值才写）；
//   读：pic 首项进 `pic`、其余进 `pics`（长度 >1 才有），model 可能 undefined；
//   center/size 走**硬**数组兜底；`sound` 走 `one_or_arr`；velocity 信息就地并进 ret；
//   next/on_dead/on_landing/on_exhaustion、bdy/itr/opoint 三串、wpoint/bpoint/cpoint/
//   chase 走 `merge_by_tag`（**没有子元素就是 undefined**，不吃缺省）、hit/hold/key_down/
//   key_up/seqs 五组击键映射、dataset 走 `xml_to_world_dataset`；尾部 `delete_undefined`。
std::shared_ptr<IXMLElement> xml_x_frame(IXML& xml, const Value& f, const std::u16string& tag);
Value xml_2_frame(const IXMLElement& el);

}
}
}
