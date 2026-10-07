#include "lfw/dat_translator/xml/xml_x_non_empty.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_arr(const IXMLElement& el, const std::u16string& tag,
                const XmlElementParser& parser) {
  auto arr = std::make_shared<Array>();
  for (IXMLElement* child : el.children_by_tag(tag)) arr->push_back(parser(*child));
  return Value(std::move(arr));
}

Value xml_2_non_empty(const IXMLElement& el, const std::u16string& tag,
                      const XmlElementParser& parser) {
  const Value ret = xml_2_arr(el, tag, parser);
  const Array* a = as_array(ret);
  if (a != nullptr && a->size() != 0) return ret;
  return Value();
}

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_non_empty(
    IXML& xml, const Value& arr, const std::u16string& tag, const XmlElementCreator& creator,
    const std::shared_ptr<IXMLElement>& parent) {
  const Array* a = as_array(arr);
  if (a == nullptr || a->size() == 0) return std::nullopt;
  std::vector<std::shared_ptr<IXMLElement>> ret;
  for (size_t i = 0; i < a->size(); ++i) {
    std::shared_ptr<IXMLElement> e = creator(xml, a->at(i), tag);
    if (e) ret.push_back(std::move(e));
  }
  if (parent) {
    for (const std::shared_ptr<IXMLElement>& e : ret) parent->insert(e);
  }
  return ret;
}

}
}
}
