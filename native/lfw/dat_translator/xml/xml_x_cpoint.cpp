#include "lfw/dat_translator/xml/xml_x_cpoint.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_next_frame.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

std::optional<double> at(const std::optional<std::vector<std::optional<double>>>& arr,
                         size_t i) {
  if (!arr || i >= arr->size()) return std::nullopt;
  return (*arr)[i];
}

}

std::shared_ptr<IXMLElement> xml_x_cpoint(IXML& xml, const Value& i, const std::u16string& tag) {
  if (!truthy(i)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"kind", field_or(i, u"kind"));
  {
    auto pos = std::make_shared<Array>();
    pos->push_back(field_or(i, u"x"));
    pos->push_back(field_or(i, u"y"));
    pos->push_back(field_or(i, u"z"));
    ret->set_arr_attr_soft(u"pos", Value(std::move(pos)));
  }
  for (const std::shared_ptr<IXMLElement>& v :
       xml_x_t_next_frame(xml, field_or(i, u"vaction"), u"vaction")) {
    ret->insert(v);
  }
  ret->set_attr(u"injury", field_or(i, u"injury"));
  ret->set_attr(u"hurtable", field_or(i, u"hurtable"));
  ret->set_attr(u"decrease", field_or(i, u"decrease"));
  {
    auto throwv = std::make_shared<Array>();
    throwv->push_back(field_or(i, u"throwvx"));
    throwv->push_back(field_or(i, u"throwvy"));
    throwv->push_back(field_or(i, u"throwvz"));
    ret->set_arr_attr_soft(u"throwv", Value(std::move(throwv)));
  }
  ret->set_attr(u"throwinjury", field_or(i, u"throwinjury"));
  ret->set_attr(u"fronthurtact", field_or(i, u"fronthurtact"));
  ret->set_attr(u"backhurtact", field_or(i, u"backhurtact"));
  ret->set_attr(u"shaking", field_or(i, u"shaking"));
  ret->set_attr(u"motionless", field_or(i, u"motionless"));
  return ret;
}

Value xml_2_cpoint(const IXMLElement& el) {
  Value ret = cpoint_new();
  Object* const o = as_object(ret);
  const std::optional<std::vector<std::optional<double>>> pos = el.nums_attr_soft(u"pos");
  const std::optional<std::vector<std::optional<double>>> throwv = el.nums_attr_soft(u"throwv");
  o->set(u"kind", from_opt(get_num_or(el, u"kind", field_or(*o, u"kind"))));
  {
    std::optional<double> fallback = at(pos, 0);
    if (!fallback) fallback = opt_num(field_or(*o, u"x"));
    o->set(u"x", from_opt(get_num_or(el, u"x", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(pos, 1);
    if (!fallback) fallback = opt_num(field_or(*o, u"y"));
    o->set(u"y", from_opt(get_num_or(el, u"y", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(pos, 2);
    if (!fallback) fallback = opt_num(field_or(*o, u"z"));
    o->set(u"z", from_opt(get_num_or(el, u"z", from_opt(fallback))));
  }
  o->set(u"vaction", xml_2_t_next_frame(el.children_by_tag(u"vaction")));
  o->set(u"injury", from_opt(get_num_or(el, u"injury", field_or(*o, u"injury"))));
  o->set(u"hurtable", from_opt(get_num_or(el, u"hurtable", field_or(*o, u"hurtable"))));
  o->set(u"decrease", from_opt(get_num_or(el, u"decrease", field_or(*o, u"decrease"))));
  {
    std::optional<double> fallback = at(throwv, 0);
    if (!fallback) fallback = opt_num(field_or(*o, u"throwvx"));
    o->set(u"throwvx", from_opt(get_num_or(el, u"throwvx", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(throwv, 1);
    if (!fallback) fallback = opt_num(field_or(*o, u"throwvy"));
    o->set(u"throwvy", from_opt(get_num_or(el, u"throwvy", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(throwv, 2);
    if (!fallback) fallback = opt_num(field_or(*o, u"throwvz"));
    o->set(u"throwvz", from_opt(get_num_or(el, u"throwvz", from_opt(fallback))));
  }
  o->set(u"fronthurtact", from_opt(get_str_or(el, u"fronthurtact", field_or(*o, u"fronthurtact"))));
  o->set(u"backhurtact", from_opt(get_str_or(el, u"backhurtact", field_or(*o, u"backhurtact"))));
  o->set(u"shaking", from_opt(get_num_or(el, u"shaking", field_or(*o, u"shaking"))));
  o->set(u"motionless", from_opt(get_num_or(el, u"motionless", field_or(*o, u"motionless"))));
  delete_undefined(ret);
  reorder_fields(ret, cpoint_info_fields());
  return ret;
}

}
}
}
