#include "lfw/dat_translator/xml/xml_x_armor_info.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_armor_info(IXML& xml, const Value& a, const std::u16string& tag) {
  if (!truthy(a)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(a, u"id"));
  ret->set_attr(u"name", field_or(a, u"name"));
  ret->set_attr(u"type", field_or(a, u"type"));
  ret->set_attr(u"toughness", field_or(a, u"toughness"));
  ret->set_attr(u"fireproof", field_or(a, u"fireproof"));
  ret->set_attr(u"antifreeze", field_or(a, u"antifreeze"));
  ret->set_attr(u"fulltime", field_or(a, u"fulltime"));
  ret->set_attr(u"injury_ratio", field_or(a, u"injury_ratio"));
  ret->set_attr(u"shaking_ratio", field_or(a, u"shaking_ratio"));
  ret->set_attr(u"motionless_ratio", field_or(a, u"motionless_ratio"));
  ret->set_attr(u"hit_sounds", field_or(a, u"hit_sounds"));
  ret->set_attr(u"dead_sounds", field_or(a, u"dead_sounds"));
  return ret;
}

Value xml_2_armor_info(const IXMLElement* el) {
  if (el == nullptr) return Value();
  Value ret = armor_Info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(el->get_str(u"id")));
  o->set(u"name", from_opt(el->get_str(u"name")));
  o->set(u"type", from_opt(el->get_num(u"type")));
  o->set(u"toughness", from_opt(el->get_num(u"toughness")));
  o->set(u"fireproof", from_opt(el->get_num(u"fireproof")));
  o->set(u"antifreeze", from_opt(el->get_num(u"antifreeze")));
  o->set(u"fulltime", from_opt(el->get_bool(u"fulltime")));
  o->set(u"injury_ratio", from_opt(el->get_num(u"injury_ratio")));
  o->set(u"shaking_ratio", from_opt(el->get_num(u"shaking_ratio")));
  o->set(u"motionless_ratio", from_opt(el->get_num(u"motionless_ratio")));
  o->set(u"hit_sounds", from_opt(el->get_str_arr(u"hit_sounds")));
  o->set(u"dead_sounds", from_opt(el->get_str_arr(u"dead_sounds")));
  return delete_undefined(ret);
}

}
}
}
