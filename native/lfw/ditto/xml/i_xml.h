#pragma once

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/ditto/xml/i_xml_element.h"

namespace lfw {

// Mirrors `src/LFW/ditto/xml/IXML.ts`（宿主服务接口，同 `IXMLElement`）。
//
// 端口偏差：`parse(text)` 的语义来自 fast-xml-parser（tool 实现）／浏览器 `DOMParser`，
// 两边的细节都不小 ⇒ 本刀**还没搬**（`ToolXML::parse` 给 `nullptr`，见其注释）；用例按
// 「用 `create` / `from_*` 搭树」的口径走，读真实文件的那条路等 parse 的刀。
class IXML {
 public:
  virtual ~IXML() = default;

  virtual std::shared_ptr<IXMLElement> parse(const std::u16string& text) = 0;
  // `create(tag, text?)`：浏览器版能顺手写 text，但 LFW 里没有两参调用点（tool 版本来就
  // 是单参）⇒ 端口只留单参。
  virtual std::shared_ptr<IXMLElement> create(const std::u16string& tag) = 0;
  virtual std::shared_ptr<IXMLElement> from_string(const std::u16string& str,
                                                   const std::u16string& tag = u"string") = 0;
  virtual std::shared_ptr<IXMLElement> from_number(double num,
                                                   const std::u16string& tag = u"number") = 0;
  virtual std::shared_ptr<IXMLElement> from_boolean(bool v,
                                                    const std::u16string& tag = u"boolean") = 0;
  virtual std::shared_ptr<IXMLElement> from_array(const Value& arr,
                                                  const std::u16string& tag = u"array") = 0;
  virtual std::shared_ptr<IXMLElement> from_object(const Value& obj,
                                                   const std::u16string& tag = u"object") = 0;
};

}
