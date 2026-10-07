#include "lfw/dat_translator/xml/xml_x_stage_object_info.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_difficulty_map.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_stage_object_info(IXML& xml, const Value& o,
                                                     const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"id", field_or(o, u"id"));
  el->set_attr(u"id_method", field_or(o, u"id_method"));
  el->set_attr(u"x", field_or(o, u"x"));
  el->set_attr(u"y", field_or(o, u"y"));
  el->set_attr(u"z", field_or(o, u"z"));
  el->set_attr(u"range_x", field_or(o, u"range_x"));
  el->set_attr(u"range_y", field_or(o, u"range_y"));
  el->set_attr(u"range_z", field_or(o, u"range_z"));
  el->set_attr(u"act", field_or(o, u"act"));
  el->set_attr(u"facing", field_or(o, u"facing"));
  el->set_attr(u"hp", field_or(o, u"hp"));
  el->set_attr(u"mp", field_or(o, u"mp"));
  xml_x_difficulty_map(el, u"hp", field_or(o, u"hp_map"));
  xml_x_difficulty_map(el, u"mp", field_or(o, u"mp_map"));
  el->set_attr(u"times", field_or(o, u"times"));
  el->set_attr(u"ratio", field_or(o, u"ratio"));
  el->set_attr(u"is_boss", field_or(o, u"is_boss"));
  el->set_attr(u"is_soldier", field_or(o, u"is_soldier"));
  el->set_attr(u"reserve", field_or(o, u"reserve"));
  el->set_attr(u"join", field_or(o, u"join"));
  el->set_attr(u"join_team", field_or(o, u"join_team"));
  el->set_attr(u"join_reserve", field_or(o, u"join_reserve"));
  el->set_attr(u"outline_color", field_or(o, u"outline_color"));
  return el;
}

Value xml_2_stage_object_info(const IXMLElement& el) {
  Value ret = stage_object_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(el.get_str_arr(u"id")));
  o->set(u"id_method", from_opt(get_str_or(el, u"id_method", field_or(*o, u"id_method"))));
  o->set(u"x", from_opt(get_num_or(el, u"x", field_or(*o, u"x"))));
  o->set(u"y", from_opt(get_num_or(el, u"y", field_or(*o, u"y"))));
  o->set(u"z", from_opt(get_num_or(el, u"z", field_or(*o, u"z"))));
  o->set(u"range_x", from_opt(get_num_or(el, u"range_x", field_or(*o, u"range_x"))));
  o->set(u"range_y", from_opt(get_num_or(el, u"range_y", field_or(*o, u"range_y"))));
  o->set(u"range_z", from_opt(get_num_or(el, u"range_z", field_or(*o, u"range_z"))));
  o->set(u"act", from_opt(get_str_or(el, u"act", field_or(*o, u"act"))));
  o->set(u"facing", from_opt(get_num_or(el, u"facing", field_or(*o, u"facing"))));
  o->set(u"hp", from_opt(get_num_or(el, u"hp", field_or(*o, u"hp"))));
  o->set(u"mp", from_opt(get_num_or(el, u"mp", field_or(*o, u"mp"))));
  o->set(u"hp_map", xml_2_difficulty_map(el, u"hp"));
  o->set(u"mp_map", xml_2_difficulty_map(el, u"mp"));
  o->set(u"times", from_opt(get_num_or(el, u"times", field_or(*o, u"times"))));
  o->set(u"ratio", from_opt(get_num_or(el, u"ratio", field_or(*o, u"ratio"))));
  o->set(u"is_boss", from_opt(get_bool_or(el, u"is_boss", field_or(*o, u"is_boss"))));
  o->set(u"is_soldier", from_opt(get_bool_or(el, u"is_soldier", field_or(*o, u"is_soldier"))));
  o->set(u"reserve", from_opt(get_num_or(el, u"reserve", field_or(*o, u"reserve"))));
  o->set(u"join", from_opt(get_num_or(el, u"join", field_or(*o, u"join"))));
  o->set(u"join_team", from_opt(get_str_or(el, u"join_team", field_or(*o, u"join_team"))));
  o->set(u"join_reserve",
         from_opt(get_num_or(el, u"join_reserve", field_or(*o, u"join_reserve"))));
  o->set(u"outline_color",
         from_opt(get_str_or(el, u"outline_color", field_or(*o, u"outline_color"))));
  delete_undefined(ret);
  reorder_fields(ret, stage_object_info_fields());
  return ret;
}

}
}
}
