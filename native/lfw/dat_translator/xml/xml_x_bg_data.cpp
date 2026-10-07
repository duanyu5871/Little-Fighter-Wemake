#include "lfw/dat_translator/xml/xml_x_bg_data.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/merge_by_tag.h"
#include "lfw/dat_translator/xml/xml_to_bg_terrain.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_bg_info.h"
#include "lfw/dat_translator/xml/xml_x_bg_layer.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_partial_world_dataset.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_bg_data(const IXMLElement& el) {
  Value ret = bg_data_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"alias_id", from_opt(get_str_or(el, u"alias_id", field_or(*o, u"alias_id"))));
  o->set(u"type", Value(u"background"));
  o->set(u"base", merge_by_tag(el, u"base", xml_2_bg_info, field_or(*o, u"base")));
  o->set(u"dataset",
         merge_by_tag(el, u"dataset",
                      [](const IXMLElement& e) { return xml_2_partial_world_dataset(&e); },
                      field_or(*o, u"dataset")));
  o->set(u"layers", xml_2_non_empty(el, u"layer", xml_2_bg_layer));
  o->set(u"terrain", xml_2_non_empty(el, u"terrain", xml_to_bg_terrain));
  delete_undefined(ret);
  reorder_fields(ret, bg_data_fields());
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_bg_data(IXML& xml, const Value& data,
                                           const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(data, u"id"));
  ret->insert(xml_x_bg_info(xml, field_or(data, u"base"), u"base"));
  ret->insert(xml_x_partial_world_dataset(xml, field_or(data, u"dataset"), u"dataset"));
  xml_x_non_empty(xml, field_or(data, u"layers"), u"layer", xml_x_bg_layer, ret);
  return ret;
}

}
}
}
