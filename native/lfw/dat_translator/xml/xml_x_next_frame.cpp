#include "lfw/dat_translator/xml/xml_x_next_frame.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/one_or_arr.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_next_frame(IXML& xml, const Value& i, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(i, u"id"));
  ret->set_attr(u"wait", field_or(i, u"wait"));
  ret->set_attr(u"facing", field_or(i, u"facing"));
  const Value expression = field_or(i, u"expression");
  if (truthy(expression)) {
    std::shared_ptr<IXMLElement> el = xml.create(u"expression");
    el->set_attr(u"value", expression);
    ret->insert(el);
  }
  ret->set_attr(u"mp", field_or(i, u"mp"));
  ret->set_attr(u"mp_mode", field_or(i, u"mp_mode"));
  ret->set_attr(u"hp", field_or(i, u"hp"));
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(i, u"dvx"));
    arr->push_back(field_or(i, u"dvy"));
    arr->push_back(field_or(i, u"dvz"));
    ret->set_arr_attr_soft(u"dv", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(i, u"acc_x"));
    arr->push_back(field_or(i, u"acc_y"));
    arr->push_back(field_or(i, u"acc_z"));
    ret->set_arr_attr_soft(u"acc", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(i, u"vxm"));
    arr->push_back(field_or(i, u"vym"));
    arr->push_back(field_or(i, u"vzm"));
    ret->set_arr_attr_soft(u"vm", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(i, u"ctrl_x"));
    arr->push_back(field_or(i, u"ctrl_y"));
    arr->push_back(field_or(i, u"ctrl_z"));
    ret->set_arr_attr_soft(u"ctrl", Value(std::move(arr)));
  }
  ret->set_attr(u"sound", field_or(i, u"sound"));
  ret->set_attr(u"blink_time", field_or(i, u"blink_time"));
  return ret;
}

Value xml_2_next_frame(const IXMLElement& el) {
  Value ret = next_frame_new();
  Object* const o = as_object(ret);
  o->set(u"id", one_or_arr(el.get_str_arr(u"id")));
  o->set(u"desc", from_opt(get_str_or(el, u"desc", field_or(*o, u"desc"))));
  {
    const std::optional<double> n = el.get_num(u"wait");
    o->set(u"wait", n ? Value(*n) : from_opt(el.get_str(u"wait")));
  }
  o->set(u"facing", from_opt(get_num_or(el, u"facing", field_or(*o, u"facing"))));
  o->set(u"expression", from_opt(get_str_or(el, u"expression", field_or(*o, u"expression"))));
  o->set(u"mp", from_opt(get_num_or(el, u"mp", field_or(*o, u"mp"))));
  o->set(u"mp_mode", from_opt(get_num_or(el, u"mp_mode", field_or(*o, u"mp_mode"))));
  o->set(u"hp", from_opt(get_num_or(el, u"hp", field_or(*o, u"hp"))));
  o->set(u"sound", non_empty(el.get_str_arr(u"sound")));
  o->set(u"blink_time", from_opt(get_num_or(el, u"blink_time", field_or(*o, u"blink_time"))));
  return delete_undefined(ret);
}

std::vector<std::shared_ptr<IXMLElement>> xml_x_t_next_frame(
    IXML& xml, const Value& nf, const std::u16string& tag,
    const std::shared_ptr<IXMLElement>& parent) {
  std::vector<std::shared_ptr<IXMLElement>> ret;
  if (!truthy(nf)) return ret;
  std::vector<const Value*> nfs;
  if (const Array* a = as_array(nf)) {
    for (size_t i = 0; i < a->size(); ++i) nfs.push_back(&a->at(i));
  } else {
    nfs.push_back(&nf);
  }
  for (const Value* v : nfs) {
    std::shared_ptr<IXMLElement> el = xml_x_next_frame(xml, *v, tag);
    if (parent) parent->insert(el);
    ret.push_back(std::move(el));
  }
  return ret;
}

Value xml_2_t_next_frame(const std::vector<IXMLElement*>& els) {
  auto arr = std::make_shared<Array>();
  for (IXMLElement* e : els) arr->push_back(xml_2_next_frame(*e));
  if (arr->size() > 1) return Value(std::move(arr));
  if (arr->size() == 0) return Value();
  return arr->at(0);
}

}
}
}
