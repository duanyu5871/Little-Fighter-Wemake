#pragma once

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/parse_rect_qube.ts`：
// 快捷属性 `rect="x,y,w,h[,z[,l]]"` / `qube="…"`（先 rect 后 qube、长度 >=4 才算）；
// 给对象 `{x,y,w,h,z,l}` —— **z/l 可能带值 undefined**（JS 字面量原样），
// 两个都没有 ⇒ 空对象 `{}`（不是 undefined）。
Value parse_rect_qube(const IXMLElement& el);

}
}
}
