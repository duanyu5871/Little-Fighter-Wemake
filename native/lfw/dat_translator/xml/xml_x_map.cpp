#include "lfw/dat_translator/xml/xml_x_map.h"

#include <variant>

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}  // namespace

Value xml_2_map(const IXMLElement& el, const std::vector<std::u16string>& tags,
                const XmlMapReader& reader) {
  auto ret = std::make_shared<Object>();
  for (const std::u16string& tag : tags) {
    for (IXMLElement* child : el.children_by_tag(tag)) {
      Value value = reader(*child);
      if (!truthy(value)) continue;
      std::optional<std::u16string> key = child->get_str(u"id");
      if (!key) key = child->get_str(u"key");
      if (!key || key->empty()) continue;
      ret->set(*key, std::move(value));
    }
  }
  if (ret->keys().empty()) return Value();
  return Value(std::move(ret));
}

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_map(
    IXML& xml, const Value& map, const std::u16string& tag, const XmlElementCreator& writer,
    const std::shared_ptr<IXMLElement>& parent) {
  std::vector<std::shared_ptr<IXMLElement>> ret;
  const Object* const m = as_object(map);
  if (m != nullptr) {
    for (const std::u16string& key : m->keys()) {
      const Value* const pv = m->get(key);
      if (pv == nullptr || is_nullish(*pv)) continue;
      std::shared_ptr<IXMLElement> el = writer(xml, *pv, tag);
      if (!el) continue;
      if (!el->get_str(u"id")) (void)el->get_str(u"id", key);
      if (!el->get_str(u"key")) (void)el->get_str(u"key", key);
      if (parent) parent->insert(el);
      ret.push_back(std::move(el));
    }
  }
  if (ret.empty()) return std::nullopt;
  return ret;
}

}
}
}
