#include "lfw/dat_translator/xml/xml_x_wpoint.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/runtime_gen.h"
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

Value xml_2_wpoint(const IXMLElement& el) {
  Value ret = wpoint_info_new();
  Object* const o = as_object(ret);
  o->set(u"kind", from_opt(get_num_or(el, u"kind", field_or(*o, u"kind"))));
  const std::optional<std::vector<std::optional<double>>> pos = el.nums_attr_soft(u"pos");
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
  o->set(u"weaponact", from_opt(get_str_or(el, u"weaponact", field_or(*o, u"weaponact"))));
  o->set(u"attacking", from_opt(get_str_or(el, u"attacking", field_or(*o, u"attacking"))));
  const std::optional<std::vector<std::optional<double>>> v = el.nums_attr_soft(u"v");
  {
    std::optional<double> fallback = at(v, 0);
    if (!fallback) fallback = opt_num(field_or(*o, u"dvx"));
    o->set(u"dvx", from_opt(get_num_or(el, u"dvx", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(v, 1);
    if (!fallback) fallback = opt_num(field_or(*o, u"dvy"));
    o->set(u"dvy", from_opt(get_num_or(el, u"dvy", from_opt(fallback))));
  }
  {
    std::optional<double> fallback = at(v, 2);
    if (!fallback) fallback = opt_num(field_or(*o, u"dvz"));
    o->set(u"dvz", from_opt(get_num_or(el, u"dvz", from_opt(fallback))));
  }
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_wpoint(IXML& xml, const Value& i, const std::u16string& tag) {
  if (!truthy(i)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"kind", field_or(i, u"kind"));
  {
    auto pos = std::make_shared<Array>();
    pos->push_back(field_or(i, u"x"));
    pos->push_back(field_or(i, u"y"));
    pos->push_back(field_or(i, u"z"));
    ret->set_attr(u"pos", Value(std::move(pos)));
  }
  ret->set_attr(u"weaponact", field_or(i, u"weaponact"));
  ret->set_attr(u"attacking", field_or(i, u"attacking"));
  {
    auto v = std::make_shared<Array>();
    v->push_back(field_or(i, u"dvx"));
    v->push_back(field_or(i, u"dvy"));
    v->push_back(field_or(i, u"dvz"));
    ret->set_arr_attr_soft(u"v", Value(std::move(v)));
  }
  return ret;
}

}
}
}
