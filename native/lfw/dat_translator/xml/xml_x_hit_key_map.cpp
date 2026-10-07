#include "lfw/dat_translator/xml/xml_x_hit_key_map.h"

#include "lfw/dat_translator/xml/xml_x_next_frame.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_hit_key_map(
    IXML& xml, const Value& map, const std::u16string& tag) {
  const Object* const m = as_object(map);
  if (m == nullptr) return std::nullopt;
  std::vector<std::shared_ptr<IXMLElement>> ret;
  for (const std::u16string& key : m->keys()) {
    const Value* const pv = m->get(key);
    if (pv == nullptr) continue;
    std::vector<std::shared_ptr<IXMLElement>> els = xml_x_t_next_frame(xml, *pv, tag);
    for (std::shared_ptr<IXMLElement>& el : els) {
      el->set_attr(u"key", Value(key));
      ret.push_back(std::move(el));
    }
  }
  if (ret.empty()) return std::nullopt;
  return ret;
}

Value xml_2_hit_key_map(const IXMLElement& el, const std::u16string& tag) {
  auto ret = std::make_shared<Object>();
  for (IXMLElement* child : el.children_by_tag(tag)) {
    std::optional<std::u16string> key = child->get_str(u"key");
    if (!key || key->empty()) continue;
    ret->set(*key, xml_2_next_frame(*child));
  }
  if (ret->keys().empty()) return Value();
  return Value(std::move(ret));
}

}
}
}
