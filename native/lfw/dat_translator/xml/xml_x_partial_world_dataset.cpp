#include "lfw/dat_translator/xml/xml_x_partial_world_dataset.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_partial_world_dataset(const IXMLElement* el) {
  if (el == nullptr) return Value();
  return el->as_object();
}

std::shared_ptr<IXMLElement> xml_x_partial_world_dataset(IXML& xml, const Value& i,
                                                         const std::u16string& tag) {
  if (!truthy(i)) return nullptr;
  const Object* const o = as_object(i);
  if (o == nullptr || o->keys().empty()) return nullptr;
  return xml.from_object(i, tag);
}

}
}
}
