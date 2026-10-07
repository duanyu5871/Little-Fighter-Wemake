#include "lfw/dat_translator/xml/xml_x_entity_data.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_bdy.h"
#include "lfw/dat_translator/xml/xml_x_entity_info.h"
#include "lfw/dat_translator/xml/xml_x_frame.h"
#include "lfw/dat_translator/xml/xml_x_frame_indexes.h"
#include "lfw/dat_translator/xml/xml_x_hit_key_map.h"
#include "lfw/dat_translator/xml/xml_x_itr.h"
#include "lfw/dat_translator/xml/xml_x_map.h"
#include "lfw/dat_translator/xml/xml_x_next_frame.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_entity_data(IXML& xml, const Value& data,
                                               const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(data, u"id"));
  ret->set_attr(u"type", field_or(data, u"type"));
  ret->set_attr(u"alias_id", field_or(data, u"alias_id"));
  ret->insert(xml_x_entity_info(xml, field_or(data, u"base"), u"base"));
  xml_x_t_next_frame(xml, field_or(data, u"on_dead"), u"on_dead", ret);
  xml_x_t_next_frame(xml, field_or(data, u"on_exhaustion"), u"on_exhaustion", ret);
  xml_x_map(xml, field_or(data, u"bdy_prefabs"), u"bdy", xml_x_bdy, ret);
  xml_x_map(xml, field_or(data, u"itr_prefabs"), u"itr", xml_x_itr, ret);
  xml_x_map(xml, field_or(data, u"frame_prefabs"), u"frame_prefab", xml_x_frame, ret);
  ret->insert(xml_x_frame_indexes(xml, field_or(data, u"indexes"), u"indexes"));
  ret->set_attr(u"processed", field_or(data, u"processed"));
  {
    const std::optional<std::vector<std::shared_ptr<IXMLElement>>> pre =
        xml_x_hit_key_map(xml, field_or(data, u"pre_hitkeys"), u"pre_hitkey");
    if (pre) {
      for (const std::shared_ptr<IXMLElement>& el : *pre) ret->insert(el);
    }
  }
  {
    const std::optional<std::vector<std::shared_ptr<IXMLElement>>> post =
        xml_x_hit_key_map(xml, field_or(data, u"post_hitkeys"), u"post_hitkey");
    if (post) {
      for (const std::shared_ptr<IXMLElement>& el : *post) ret->insert(el);
    }
  }
  xml_x_map(xml, field_or(data, u"frames"), u"frame", xml_x_frame, ret);
  return ret;
}

Value xml_2_entity_data(const IXMLElement* el) {
  if (el == nullptr) return Value();
  Value ret = entity_data_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(*el, u"id", field_or(*o, u"id"))));
  o->set(u"type", from_opt(get_num_or(*el, u"type", field_or(*o, u"type"))));
  o->set(u"alias_id", from_opt(get_str_or(*el, u"alias_id", field_or(*o, u"alias_id"))));
  // TS: `xml_2_entity_info(el.child_by_tag("base")!)` —— 契约上必有 base 子元素。
  o->set(u"base", xml_2_entity_info(*el->child_by_tag(u"base")));
  o->set(u"on_dead", xml_2_t_next_frame(el->children_by_tag(u"on_dead")));
  o->set(u"on_exhaustion", xml_2_t_next_frame(el->children_by_tag(u"on_exhaustion")));
  o->set(u"indexes", xml_2_frame_indexes(el->child_by_tag(u"indexes")));
  o->set(u"bdy_prefabs", xml_2_map(*el, {u"bdy_prefab", u"bdy"}, xml_2_bdy));
  o->set(u"itr_prefabs", xml_2_map(*el, {u"itr_prefab", u"itr"}, xml_2_itr));
  o->set(u"frame_prefabs", xml_2_map(*el, {u"frame_prefab"}, xml_2_frame));
  {
    // `el.get_bool("processed", ret.processed) || void 0`
    const std::optional<bool> processed = get_bool_or(*el, u"processed", field_or(*o, u"processed"));
    o->set(u"processed", processed && *processed ? Value(true) : Value());
  }
  o->set(u"pre_hitkeys", xml_2_hit_key_map(*el, u"pre_hitkey"));
  o->set(u"post_hitkeys", xml_2_hit_key_map(*el, u"post_hitkey"));
  {
    // `xml_2_map(el, "frame", xml_2_frame) ?? {}` —— 缺了给空对象。
    const Value frames = xml_2_map(*el, {u"frame"}, xml_2_frame);
    if (truthy(frames)) {
      o->set(u"frames", frames);
    } else {
      o->set(u"frames", Value(std::make_shared<Object>()));
    }
  }
  delete_undefined(ret);
  reorder_fields(ret, entity_data_fields());
  return ret;
}

}
}
}
