#include "lfw/dat_translator/xml/xml_x_bdy.h"

#include <optional>
#include <string>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_colli_action.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_qube.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_bdy(IXML& xml, const Value& b, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(b, u"id"));
  ret->set_attr(u"name", field_or(b, u"name"));
  ret->set_attr(u"ref", field_or(b, u"ref"));
  ret->set_attr(u"kind", field_or(b, u"kind"));
  ret->set_attr(u"hit_flag", field_or(b, u"hit_flag"));
  {
    auto qube = std::make_shared<Array>();
    qube->push_back(field_or(b, u"x"));
    qube->push_back(field_or(b, u"y"));
    qube->push_back(field_or(b, u"w"));
    qube->push_back(field_or(b, u"h"));
    qube->push_back(field_or(b, u"z"));
    qube->push_back(field_or(b, u"l"));
    ret->set_arr_attr_soft(u"qube", Value(std::move(qube)));
  }
  xml_x_non_empty(xml, field_or(b, u"actions"), u"action", xml_x_colli_action, ret);
  ret->set_attr(u"test", field_or(b, u"test"));
  ret->set_attr(u"code", field_or(b, u"code"));
  return ret;
}

Value xml_2_bdy(const IXMLElement& el) {
  Value ret = bdy_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  {
    std::optional<std::u16string> v = el.get_str(u"ref");
    if (!v) v = el.get_str(u"prefab_id");
    if (!v) v = opt_str(field_or(*o, u"ref"));
    o->set(u"ref", from_opt(v));
  }
  o->set(u"kind", from_opt(get_num_or(el, u"kind", field_or(*o, u"kind"))));
  o->set(u"hit_flag", from_opt(get_num_or(el, u"hit_flag", field_or(*o, u"hit_flag"))));
  xml_2_qube(el, ret);
  o->set(u"actions", xml_2_non_empty(el, u"action", xml_2_colli_action));
  o->set(u"test", from_opt(get_str_or(el, u"test", field_or(*o, u"test"))));
  o->set(u"code", from_opt(get_num_or(el, u"code", field_or(*o, u"code"))));
  return delete_undefined(ret);
}

}
}
}
