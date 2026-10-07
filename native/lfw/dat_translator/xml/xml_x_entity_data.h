#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_entity_data.ts`（tag 缺省 `entity`）：
//   写：id/type/alias_id，base 实体信息，on_dead/on_exhaustion 下一帧串，bdy/itr/
//   frame_prefab 三组 map，indexes，processed，pre/post_hitkey 两组（返回列表逐个 insert），
//   最后 frames map；
//   读：`nullptr` 当 undefined；`base` 直接 `xml_2_entity_info(*el.child_by_tag("base"))`
//   （TS 有 `!` 断言 ⇒ 契约上必有）；`processed` 是 `get_bool(...) || void 0`（假值给
//   undefined）；`frames` 缺省 `{}`（**对象**，不是 undefined）；bdy/itr/frame_prefab 用
//   多 tag 查找（["bdy_prefab","bdy"] 等）；尾部删除 + 按字段表重排。
std::shared_ptr<IXMLElement> xml_x_entity_data(IXML& xml, const Value& data,
                                               const std::u16string& tag = u"entity");
Value xml_2_entity_data(const IXMLElement* el);

}
}
}
