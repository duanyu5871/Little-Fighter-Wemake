#include "lfw/dat_translator/xml/xml_x_picture_info.h"

#include <vector>

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_map.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_picture_info(const IXMLElement& el) {
  Value ret = picture_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"path", from_opt(get_str_or(el, u"path", field_or(*o, u"path"))));
  o->set(u"variants", from_opt(el.get_str_arr(u"variants")));
  o->set(u"row", from_opt(get_num_or(el, u"row", field_or(*o, u"row"))));
  o->set(u"col", from_opt(get_num_or(el, u"col", field_or(*o, u"col"))));
  o->set(u"cell_w", from_opt(get_num_or(el, u"cell_w", field_or(*o, u"cell_w"))));
  o->set(u"cell_h", from_opt(get_num_or(el, u"cell_h", field_or(*o, u"cell_h"))));
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_picture_info(IXML& xml, const Value& f,
                                                const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(f, u"id"));
  ret->set_attr(u"path", field_or(f, u"path"));
  ret->set_attr(u"variants", field_join(field_or(f, u"variants")));
  ret->set_attr(u"row", field_or(f, u"row"));
  ret->set_attr(u"col", field_or(f, u"col"));
  ret->set_attr(u"cell_w", field_or(f, u"cell_w"));
  ret->set_attr(u"cell_h", field_or(f, u"cell_h"));
  return ret;
}

Value xml_2_picture_info_map(const IXMLElement& el, const std::u16string& tag) {
  return xml_2_map(el, {tag}, xml_2_picture_info);
}

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_picture_info_map(
    IXML& xml, const Value& map, const std::u16string& tag) {
  return xml_x_map(xml, map, tag, xml_x_picture_info);
}

}
}
}
