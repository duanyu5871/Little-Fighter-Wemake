#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// Mirrors `src/LFW/dat_translator/xml/xml_from_json.ts`（手写的 JSON → XML 文本）：
//   `esc` 只换 `< > & "` 四个（**没有** `'`）；`String(v)` 语义走 `to_string`。
//   `attrsOf`：按 keyOrder（缺省 `Object.keys`）过滤 —— `undefined` / `null` /
//   **`typeof === "object"`（数组也算！）** 全跳过 ⇒ 数组根本进不了属性；它下面那行
//   `Array.isArray(v) && v.some(x => typeof x === "object")` 与 `attrs` 里的数组分支
//   在 TS 里都是**死代码**（照抄语义、注释保留）。
//   `childrenXml`：undefined/null 跳过；数组先 `every(x => typeof x !== "object")`
//   —— 全是原始值的数组**在 attrsOf 里也被扔了**（TS 注释说「已在属性里处理」，实际不是）；
//   否则逐项：对象项（非 null）用 `item.tagName ?? key` 当标签、`attrsOf(item)` 造属性，
//   有任何「非 nullish 的对象」值时展开子层、否则自闭合；原始值项（含 null）输出
//   `<key>esc(item)</key>`（null 变成字面量 `null`）。对象值项同理（`tagName` 只看数组项）。
//   根：`<tagName attrsOf(data)>\n` + children + `</tagName>\n`（keyOrder 只影响属性顺序，
//   子层遍历仍是原对象的键序）。
std::u16string xml_from_json(const Value& data, const std::u16string& tag_name,
                             const std::optional<std::vector<std::u16string>>& key_order =
                                 std::nullopt);

}
}
}
