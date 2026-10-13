#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml.h"
#include "lfw/ditto/xml/tool_xml_element.h"

namespace lfw {

// Mirrors `tool/src/xml/ToolXML.ts`：工厂 + 解析（Node 侧那份的端口）。
// 语义细节（`from_number` 写文本而非 `value` 属性等）见 `ToolXMLElement` 的头部注释。
// 页面里的全局单例（TS `export const XML`）不在端口里给——LFW 侧按宿主 seam 拿 `IXML&`。
class ToolXML : public IXML {
 public:
  // XML 文本 → 元素树（**已搬**）。语义口径来自 fast-xml-parser（`tool/src/xml/ToolXML.ts` 的
  // 那套选项：`ignoreAttributes:false` / `attributeNamePrefix:'@_'` / `ignoreDeclaration:true` /
  // `removeNSPrefix:true` / `allowBooleanAttributes:true` / `parseTagValue:false` /
  // `trimValues:false`）：
  //   * 元素只有直接文本（无属性、无子元素）⇒ 文本；空元素 ⇒ 空串；
  //   * 有属性 ⇒ 属性（无值属性是 `"true"`），直接文本进 `#text`；
  //   * 直接文本**保留空白**并按序拼接；子元素按文档序；`ns:` 前缀去掉；
  //   * 只解 `&amp; &lt; &gt; &quot; &apos;` 五个命名实体（数字引用照抄，实测 fast-xml-parser
  //     在这个配置下就不解它们）；`<![CDATA[…]]>` 原样当文本；注释先剥掉；
  //   * 一个顶层元素都没有 ⇒ TS 抛 `No root element found in XML`，端口给 `nullptr`。
  std::shared_ptr<IXMLElement> parse(const std::u16string& text) override;
  std::shared_ptr<IXMLElement> create(const std::u16string& tag) override;
  std::shared_ptr<IXMLElement> from_string(const std::u16string& str,
                                           const std::u16string& tag) override;
  std::shared_ptr<IXMLElement> from_number(double num, const std::u16string& tag) override;
  std::shared_ptr<IXMLElement> from_boolean(bool v, const std::u16string& tag) override;
  std::shared_ptr<IXMLElement> from_array(const Value& arr, const std::u16string& tag) override;
  std::shared_ptr<IXMLElement> from_object(const Value& obj, const std::u16string& tag) override;

 private:
  // `_from_value`：`null` / `undefined` → 只有 `type` 属性的 `<string/>`，
  // 否则按 JS 类型分派到 `from_*`（tag 参数在 tool 实现里本来就被忽略）。
  std::shared_ptr<IXMLElement> from_value(const Value& value);
};

}
