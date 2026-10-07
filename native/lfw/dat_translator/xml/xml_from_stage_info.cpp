#include "lfw/dat_translator/xml/xml_from_stage_info.h"

#include "lfw/dat_translator/xml/xml_x_stage_info.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::u16string xml_from_stage_info(IXML& xml, const Value& stages) {
  std::shared_ptr<IXMLElement> root = xml.create(u"stages");
  const Array* const a = as_array(stages);
  if (a != nullptr) {
    for (size_t i = 0; i < a->size(); ++i) root->insert(xml_x_stage_info(xml, a->at(i), u"stage"));
  }
  return root->stringify();
}

}
}
}
