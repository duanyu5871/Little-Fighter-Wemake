#pragma once

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_x_qube.ts` 的 `xml_2_qube(el, out)`：
// 六个分量各自 `el.get_num(键, rect[i] ?? qube[i] ?? out[键])`；随后
// `delete_undefined(out)`（先删旧值里的 undefined 键）再把六个键**整体写回**
// （值可能是 undefined ⇒ 键会带 undefined 值，由调用方的 `delete_undefined` 收尾）。
// `out` 就地改（`Value` 里的 `Object` 是共享的），返回同一个 `out`。
Value xml_2_qube(const IXMLElement& el, Value out);

}
}
}
