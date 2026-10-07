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

// Mirrors `src/LFW/dat_translator/xml/xml_x_frame_pic.ts`：
//   读：`tex` 带缺省；`rect="x,y,w,h"` 硬拆前四项当 x/y/w/h 的备选（`rect?.[i] ?? 缺省`），
//   `deg/rad/ox/oy/cx/cy` 无缺省；
//   写：先 `tex`，再 `rect` = `[x,y,w,h].join()`（**恒写**，全 undefined 就是 ",,,"），
//   随后 deg/rad/ox/oy/cx/cy；另有 map 版薄壳。
Value xml_2_frame_pic(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_frame_pic(IXML& xml, const Value& pic,
                                             const std::u16string& tag);
Value xml_2_frame_pic_map(const IXMLElement& el, const std::u16string& tag);
std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_frame_pic_map(
    IXML& xml, const Value& map, const std::u16string& tag);

}
}
}
