#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_entity_info.ts`：
//   写：type/name/head/small/ce/weight/group(`?.join()`)、files/models 两组 map 造完逐个
//   insert、bounce/bounce_min/fast 三组软三元、brokens（`xml_x_opoint`）、armor/drink
//   （可能 nullptr，insert 会忽略）、drop_hurt…w_atk_r_x、portraits map（parent 直挂）、
//   最后把 `xml_from_world_dataset(xml, info, "dataset")` 插进来；
//   读：⚠ 一批 TS 级怪癖照抄 —— `bounce_min` 的软数组读成 **y←[0], x←[1], z←[2]**；
//   快速值读的是属性 `fast_v`（写向写的是 `fast`）且 [0]→vy、[1]→vx；portraits/models
//   是内联手写循环（不是 `xml_2_picture_info_map`）；brokens = broken + opoint 两串子元素；
//   `dataset` 的键**逐个盖回顶层**（`ret[k] = ds[k]`，undefined 也盖）；尾部**没有**
//   `delete_undefined`、没有重排。
std::shared_ptr<IXMLElement> xml_x_entity_info(IXML& xml, const Value& info,
                                               const std::u16string& tag);
Value xml_2_entity_info(const IXMLElement& el);

}
}
}
