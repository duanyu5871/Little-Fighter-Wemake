#include "lfw/dat_translator/xml/xml_x_opoint.h"

#include <variant>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/one_or_arr.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_next_frame.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

Value soft_at(const std::optional<std::vector<std::optional<double>>>& v, size_t i,
              const Value& or_value) {
  if (v && i < v->size() && v->at(i)) return Value(*v->at(i));
  return or_value;
}

}  // namespace

Value xml_2_opoint_multi(const IXMLElement* el) {
  if (el == nullptr) return Value();
  Value ret = opoint_multi_new();
  Object* const o = as_object(ret);
  o->set(u"type", from_opt(get_num_or(*el, u"type", field_or(*o, u"type"))));
  o->set(u"skip_zero", from_opt(get_bool_or(*el, u"skip_zero", field_or(*o, u"skip_zero"))));
  o->set(u"min", from_opt(el->get_num(u"min")));
  o->set(u"max", from_opt(el->get_num(u"max")));
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_opoint_multi(IXML& xml, const Value& o,
                                                const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"type", field_or(o, u"type"));
  ret->set_attr(u"skip_zero", field_or(o, u"skip_zero"));
  ret->set_attr(u"min", field_or(o, u"min"));
  ret->set_attr(u"max", field_or(o, u"max"));
  return ret;
}

Value xml_2_opoint(const IXMLElement& el) {
  Value ret = opoint_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(el.get_str(u"id")));
  o->set(u"name", from_opt(el.get_str(u"name")));
  o->set(u"kind", from_opt(get_num_or(el, u"kind", field_or(*o, u"kind"))));
  {
    // `one_or_arr(el.get_str_arr('oid')) ?? ''` —— 缺了给空串（不是 undefined）。
    const Value oid = one_or_arr(el.get_str_arr(u"oid"));
    o->set(u"oid", truthy(oid) ? oid : Value(std::u16string()));
  }
  const std::optional<std::vector<std::optional<double>>> pos = el.nums_attr_soft(u"pos");
  o->set(u"x", from_opt(get_num_or(el, u"x", soft_at(pos, 0, field_or(*o, u"x")))));
  o->set(u"y", from_opt(get_num_or(el, u"y", soft_at(pos, 1, field_or(*o, u"y")))));
  o->set(u"z", from_opt(get_num_or(el, u"z", soft_at(pos, 2, field_or(*o, u"z")))));
  o->set(u"pos_type", from_opt(el.get_num(u"pos_type")));
  {
    const Value action = xml_2_t_next_frame(el.children_by_tag(u"action"));
    o->set(u"action", truthy(action) ? action : field_or(*o, u"action"));
  }
  const std::optional<std::vector<std::optional<double>>> dv = el.nums_attr_soft(u"dv");
  o->set(u"dvx", from_opt(get_num_or(el, u"dvx", soft_at(dv, 0, field_or(*o, u"dvx")))));
  o->set(u"dvy", from_opt(get_num_or(el, u"dvy", soft_at(dv, 1, field_or(*o, u"dvy")))));
  o->set(u"dvz", from_opt(get_num_or(el, u"dvz", soft_at(dv, 2, field_or(*o, u"dvz")))));
  {
    const Value multi = xml_2_opoint_multi(el.child_by_tag(u"multi"));
    if (truthy(multi)) {
      o->set(u"multi", multi);
    } else {
      const std::optional<double> num = el.get_num(u"multi");
      o->set(u"multi", num ? Value(*num) : field_or(*o, u"multi"));
    }
  }
  o->set(u"max_hp", from_opt(get_num_or(el, u"max_hp", field_or(*o, u"max_hp"))));
  o->set(u"hp", from_opt(get_num_or(el, u"hp", field_or(*o, u"hp"))));
  o->set(u"max_mp", from_opt(get_num_or(el, u"max_mp", field_or(*o, u"max_mp"))));
  o->set(u"mp", from_opt(get_num_or(el, u"mp", field_or(*o, u"mp"))));
  o->set(u"speedz", from_opt(get_num_or(el, u"speedz", field_or(*o, u"speedz"))));
  o->set(u"spreading", from_opt(get_num_or(el, u"spreading", field_or(*o, u"spreading"))));
  o->set(u"ghost", from_opt(get_num_or(el, u"ghost", field_or(*o, u"ghost"))));
  o->set(u"interval", from_opt(get_num_or(el, u"interval", field_or(*o, u"interval"))));
  o->set(u"interval_id", from_opt(get_str_or(el, u"interval_id", field_or(*o, u"interval_id"))));
  o->set(u"interval_mode",
         from_opt(get_num_or(el, u"interval_mode", field_or(*o, u"interval_mode"))));
  o->set(u"motionless", from_opt(get_num_or(el, u"motionless", field_or(*o, u"motionless"))));
  o->set(u"unimportant", from_opt(get_num_or(el, u"unimportant", field_or(*o, u"unimportant"))));
  o->set(u"delay", from_opt(get_num_or(el, u"delay", field_or(*o, u"delay"))));
  o->set(u"inherit_speed_x",
         from_opt(get_num_or(el, u"inherit_speed_x", field_or(*o, u"inherit_speed_x"))));
  o->set(u"inherit_speed_y",
         from_opt(get_num_or(el, u"inherit_speed_y", field_or(*o, u"inherit_speed_y"))));
  o->set(u"inherit_speed_z",
         from_opt(get_num_or(el, u"inherit_speed_z", field_or(*o, u"inherit_speed_z"))));
  o->set(u"gen_x", from_opt(get_str_or(el, u"gen_x", field_or(*o, u"gen_x"))));
  o->set(u"gen_y", from_opt(get_str_or(el, u"gen_y", field_or(*o, u"gen_y"))));
  o->set(u"gen_z", from_opt(get_str_or(el, u"gen_z", field_or(*o, u"gen_z"))));
  o->set(u"gen_dvx", from_opt(get_str_or(el, u"gen_dvx", field_or(*o, u"gen_dvx"))));
  o->set(u"gen_dvy", from_opt(get_str_or(el, u"gen_dvy", field_or(*o, u"gen_dvy"))));
  o->set(u"gen_dvz", from_opt(get_str_or(el, u"gen_dvz", field_or(*o, u"gen_dvz"))));
  o->set(u"gen_spread_x", from_opt(get_str_or(el, u"gen_spread_x", field_or(*o, u"gen_spread_x"))));
  o->set(u"gen_spread_y", from_opt(get_str_or(el, u"gen_spread_y", field_or(*o, u"gen_spread_y"))));
  o->set(u"gen_spread_z", from_opt(get_str_or(el, u"gen_spread_z", field_or(*o, u"gen_spread_z"))));
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_opoint(IXML& xml, const Value& o, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"id", field_or(o, u"id"));
  el->set_attr(u"name", field_or(o, u"name"));
  el->set_attr(u"kind", field_or(o, u"kind"));
  el->set_attr(u"oid", field_or(o, u"oid"));
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(o, u"x"));
    arr->push_back(field_or(o, u"y"));
    arr->push_back(field_or(o, u"z"));
    el->set_arr_attr_soft(u"pos", Value(std::move(arr)));
  }
  el->set_attr(u"pos_type", field_or(o, u"pos_type"));
  {
    const std::vector<std::shared_ptr<IXMLElement>> actions =
        xml_x_t_next_frame(xml, field_or(o, u"action"), u"action");
    for (const std::shared_ptr<IXMLElement>& v : actions) el->insert(v);
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(o, u"dvx"));
    arr->push_back(field_or(o, u"dvy"));
    arr->push_back(field_or(o, u"dvz"));
    el->set_arr_attr_soft(u"dv", Value(std::move(arr)));
  }
  const Value multi = field_or(o, u"multi");
  if (std::get_if<double>(&multi) != nullptr) el->set_attr(u"multi", multi);
  {
    const Object* const mo = as_object(multi);
    if (mo != nullptr) el->insert(xml_x_opoint_multi(xml, multi, u"multi"));
  }
  el->set_attr(u"max_hp", field_or(o, u"max_hp"));
  el->set_attr(u"hp", field_or(o, u"hp"));
  el->set_attr(u"max_mp", field_or(o, u"max_mp"));
  el->set_attr(u"mp", field_or(o, u"mp"));
  el->set_attr(u"speedz", field_or(o, u"speedz"));
  el->set_attr(u"spreading", field_or(o, u"spreading"));
  el->set_attr(u"ghost", field_or(o, u"ghost"));
  el->set_attr(u"interval", field_or(o, u"interval"));
  el->set_attr(u"interval_id", field_or(o, u"interval_id"));
  el->set_attr(u"interval_mode", field_or(o, u"interval_mode"));
  el->set_attr(u"motionless", field_or(o, u"motionless"));
  el->set_attr(u"unimportant", field_or(o, u"unimportant"));
  el->set_attr(u"delay", field_or(o, u"delay"));
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(o, u"inherit_speed_x"));
    arr->push_back(field_or(o, u"inherit_speed_y"));
    arr->push_back(field_or(o, u"inherit_speed_z"));
    el->set_arr_attr_soft(u"inherit_speed", Value(std::move(arr)));
  }
  el->set_attr(u"gen_x", field_or(o, u"gen_x"));
  el->set_attr(u"gen_y", field_or(o, u"gen_y"));
  el->set_attr(u"gen_z", field_or(o, u"gen_z"));
  el->set_attr(u"gen_dvx", field_or(o, u"gen_dvx"));
  el->set_attr(u"gen_dvy", field_or(o, u"gen_dvy"));
  el->set_attr(u"gen_dvz", field_or(o, u"gen_dvz"));
  el->set_attr(u"gen_spread_x", field_or(o, u"gen_spread_x"));
  el->set_attr(u"gen_spread_y", field_or(o, u"gen_spread_y"));
  el->set_attr(u"gen_spread_z", field_or(o, u"gen_spread_z"));
  return el;
}

}
}
}
