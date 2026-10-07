#include "lfw/dat_translator/xml/xml_x_colli_action.h"

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_colli_action(IXML& xml, const Value& action,
                                                const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"type", field_or(action, u"type"));
  ret->set_attr(u"test", field_or(action, u"test"));
  ret->set_attr(u"pretest", field_or(action, u"pretest"));
  const std::shared_ptr<IXMLElement> data = xml.from_object(field_or(action, u"data"), u"data");
  ret->insert(data);
  return ret;
}

Value xml_2_colli_action(const IXMLElement& el) {
  auto ret = std::make_shared<Object>();
  ret->set(u"test", from_opt(el.get_str(u"test")));
  ret->set(u"pretest", from_opt(el.get_bool(u"pretest")));
  ret->set(u"type", from_opt(el.get_str(u"type")));
  ret->set(u"data", el.get_obj(u"data"));
  return Value(std::move(ret));
}

}
}
}
