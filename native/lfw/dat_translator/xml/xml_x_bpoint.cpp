#include "lfw/dat_translator/xml/xml_x_bpoint.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_bpoint(const IXMLElement& el) {
  Value ret = bpoint_info_new();
  Object* const o = as_object(ret);
  o->set(u"x", from_opt(get_num_or(el, u"x", field_or(*o, u"x"))));
  o->set(u"y", from_opt(get_num_or(el, u"y", field_or(*o, u"y"))));
  o->set(u"z", from_opt(get_num_or(el, u"z", field_or(*o, u"z"))));
  o->set(u"r", from_opt(get_num_or(el, u"r", field_or(*o, u"r"))));
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_bpoint(IXML& xml, const Value& i, const std::u16string& tag) {
  if (!truthy(i)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"x", field_or(i, u"x"));
  ret->set_attr(u"y", field_or(i, u"y"));
  ret->set_attr(u"z", field_or(i, u"z"));
  ret->set_attr(u"r", field_or(i, u"r"));
  return ret;
}

}
}
}
