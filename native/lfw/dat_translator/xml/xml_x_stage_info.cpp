#include "lfw/dat_translator/xml/xml_x_stage_info.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_stage_phase_info.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_stage_info(const IXMLElement& el) {
  Value ret = stage_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  o->set(u"bg", from_opt(get_str_or(el, u"bg", field_or(*o, u"bg"))));
  {
    // `xml_2_non_empty(el, "phase", xml_2_stage_phase_info) ?? []`
    const Value phases = xml_2_non_empty(el, u"phase", xml_2_stage_phase_info);
    if (truthy(phases)) {
      o->set(u"phases", phases);
    } else {
      o->set(u"phases", Value(std::make_shared<Array>()));
    }
  }
  o->set(u"chapter", from_opt(get_str_or(el, u"chapter", field_or(*o, u"chapter"))));
  o->set(u"next", from_opt(get_str_or(el, u"next", field_or(*o, u"next"))));
  o->set(u"cond_end", from_opt(get_str_or(el, u"cond_end", field_or(*o, u"cond_end"))));
  o->set(u"act_of_goto_next",
         from_opt(get_str_or(el, u"act_of_goto_next", field_or(*o, u"act_of_goto_next"))));
  o->set(u"is_starting", from_opt(get_bool_or(el, u"is_starting", field_or(*o, u"is_starting"))));
  o->set(u"starting_name",
         from_opt(get_str_or(el, u"starting_name", field_or(*o, u"starting_name"))));
  o->set(u"title", from_opt(get_str_or(el, u"title", field_or(*o, u"title"))));
  o->set(u"group", from_opt(el.get_str_arr(u"group")));
  delete_undefined(ret);
  reorder_fields(ret, stage_info_fields());
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_stage_info(IXML& xml, const Value& s, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"id", field_or(s, u"id"));
  el->set_attr(u"bg", field_or(s, u"bg"));
  el->set_attr(u"name", field_or(s, u"name"));
  el->set_attr(u"chapter", field_or(s, u"chapter"));
  el->set_attr(u"next", field_or(s, u"next"));
  el->set_attr(u"cond_end", field_or(s, u"cond_end"));
  el->set_attr(u"act_of_goto_next", field_or(s, u"act_of_goto_next"));
  el->set_attr(u"is_starting", field_or(s, u"is_starting"));
  el->set_attr(u"starting_name", field_or(s, u"starting_name"));
  el->set_attr(u"title", field_or(s, u"title"));
  el->set_attr(u"group", field_or(s, u"group"));
  xml_x_non_empty(xml, field_or(s, u"phases"), u"phase", xml_x_stage_phase_info, el);
  return el;
}

std::vector<Value> xml_to_stage_info_list(const IXMLElement& el) {
  std::vector<Value> ret;
  if (el.tag() == u"stages") {
    for (IXMLElement* child : el.children_by_tag(u"stage")) ret.push_back(xml_2_stage_info(*child));
    return ret;
  }
  if (el.tag() == u"stage") {
    ret.push_back(xml_2_stage_info(el));
  }
  return ret;
}

}
}
}
