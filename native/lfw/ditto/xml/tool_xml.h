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
  // 偏差：`parse` 没搬（见 `i_xml.h` 注释）。
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
