#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_frame_model.ts`（帧级 `<model>`）：
//   读：`nullptr` 当 undefined；id 带缺省，anim/seek/loop/time_scale/reverse/rad 无缺省，
//   rotation → scale → offset 三对软属性给子对象；`<pose>` 子元素在才给 `pose`，
//   bones/pos/rot/scl 逐个「有长度才给」；尾部 `delete_undefined`；
//   写：假值 ⇒ undefined；rotation/scale/offset 有子对象才写；`pose` 在才插 `<pose>`。
Value xml_2_frame_model(const IXMLElement* el);
std::shared_ptr<IXMLElement> xml_x_frame_model(IXML& xml, const Value& m,
                                               const std::u16string& tag);

}
}
}
