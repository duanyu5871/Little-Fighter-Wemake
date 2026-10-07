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

// Mirrors `src/LFW/dat_translator/xml/xml_x_model_info.ts`：
//   读：scale/offset/rotation 三对软属性 `nums_attr_soft`，只要**有一项非 nullish**就给
//   子对象（`{x,y,z}` 只装有值的那几项）；顺序 scale → offset → rotation；
//   写：id/path/variants（`?.join()`），rotation/scale/offset 有子对象才 `set_arr_attr_soft`；
//   另有 map 版两个薄壳（TS 里 `xml_2_model_info_map` 的签名说 IPictureInfo，实际用
//   `xml_2_model_info`）。
Value xml_2_model_info(const IXMLElement& el);
std::shared_ptr<IXMLElement> xml_x_model_info(IXML& xml, const Value& f,
                                              const std::u16string& tag);
Value xml_2_model_info_map(const IXMLElement& el, const std::u16string& tag);
std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_model_info_map(
    IXML& xml, const Value& map, const std::u16string& tag);

}
}
}
