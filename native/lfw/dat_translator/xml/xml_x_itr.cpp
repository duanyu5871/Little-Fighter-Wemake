#include "lfw/dat_translator/xml/xml_x_itr.h"

#include <optional>
#include <string>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_to_velocity_info.h"
#include "lfw/dat_translator/xml/xml_x_colli_action.h"
#include "lfw/dat_translator/xml/xml_x_next_frame.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_qube.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

void push_attr_t_next(IXML& xml, const std::shared_ptr<IXMLElement>& ret, const Value& i,
                      const char16_t* key) {
  for (const std::shared_ptr<IXMLElement>& v :
       xml_x_t_next_frame(xml, field_or(i, key), key)) {
    ret->insert(v);
  }
}

Value arr3(const Value& a, const Value& b, const Value& c) {
  auto arr = std::make_shared<Array>();
  arr->push_back(a);
  arr->push_back(b);
  arr->push_back(c);
  return Value(std::move(arr));
}

}

std::shared_ptr<IXMLElement> xml_x_itr(IXML& xml, const Value& i, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(i, u"id"));
  ret->set_attr(u"name", field_or(i, u"name"));
  ret->set_attr(u"ref", field_or(i, u"ref"));
  ret->set_attr(u"hit_flag", field_or(i, u"hit_flag"));
  ret->set_attr(u"motionless", field_or(i, u"motionless"));
  ret->set_attr(u"shaking", field_or(i, u"shaking"));
  ret->set_attr(u"kind", field_or(i, u"kind"));
  ret->set_attr(u"fall", field_or(i, u"fall"));
  ret->set_attr(u"vrest", field_or(i, u"vrest"));
  ret->set_attr(u"arest", field_or(i, u"arest"));
  ret->set_attr(u"bdefend", field_or(i, u"bdefend"));
  ret->set_attr(u"injury", field_or(i, u"injury"));
  ret->set_attr(u"effect", field_or(i, u"effect"));
  push_attr_t_next(xml, ret, i, u"catchingact");
  push_attr_t_next(xml, ret, i, u"caughtact");
  push_attr_t_next(xml, ret, i, u"on_hit_ground");
  xml_x_non_empty(xml, field_or(i, u"actions"), u"action", xml_x_colli_action, ret);
  {
    auto qube = std::make_shared<Array>();
    qube->push_back(field_or(i, u"x"));
    qube->push_back(field_or(i, u"y"));
    qube->push_back(field_or(i, u"w"));
    qube->push_back(field_or(i, u"h"));
    qube->push_back(field_or(i, u"z"));
    qube->push_back(field_or(i, u"l"));
    ret->set_arr_attr_soft(u"qube", Value(std::move(qube)));
  }
  ret->set_arr_attr_soft(u"dv", arr3(field_or(i, u"dvx"), field_or(i, u"dvy"),
                                     field_or(i, u"dvz")));
  ret->set_attr(u"test", field_or(i, u"test"));
  ret->set_attr(u"code", field_or(i, u"code"));
  return ret;
}

Value xml_2_itr(const IXMLElement& el) {
  Value ret = itr_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  {
    std::optional<std::u16string> v = el.get_str(u"ref");
    if (!v) v = el.get_str(u"prefab_id");
    if (!v) v = opt_str(field_or(*o, u"ref"));
    o->set(u"ref", from_opt(v));
  }
  const char16_t* const num_keys[10] = {u"hit_flag", u"motionless", u"shaking", u"kind",
                                        u"fall",     u"vrest",     u"arest",   u"bdefend",
                                        u"injury",   u"effect"};
  for (const char16_t* key : num_keys) {
    o->set(std::u16string(key),
           from_opt(get_num_or(el, std::u16string(key), field_or(*o, key))));
  }
  o->set(u"catchingact", xml_2_t_next_frame(el.children_by_tag(u"catchingact")));
  o->set(u"caughtact", xml_2_t_next_frame(el.children_by_tag(u"caughtact")));
  o->set(u"on_hit_ground", xml_2_t_next_frame(el.children_by_tag(u"on_hit_ground")));
  o->set(u"actions", xml_2_non_empty(el, u"action", xml_2_colli_action));
  o->set(u"test", from_opt(get_str_or(el, u"test", field_or(*o, u"test"))));
  o->set(u"code", from_opt(get_num_or(el, u"code", field_or(*o, u"code"))));
  xml_to_velocity_info(el, ret);
  xml_2_qube(el, ret);
  return delete_undefined(ret);
}

}
}
}
