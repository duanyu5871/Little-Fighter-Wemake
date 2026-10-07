#include "lfw/dat_translator/xml/xml_to_bg_terrain.h"

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_to_bg_terrain(const IXMLElement& el) {
  Value ret = terrain_info_new();
  Object* const o = as_object(ret);
  o->set(u"type", from_opt(get_num_or(el, u"type", field_or(*o, u"type"))));
  o->set(u"name", from_opt(el.get_str(u"name")));
  o->set(u"x1", from_opt(get_num_or(el, u"x1", field_or(*o, u"x1"))));
  o->set(u"x2", from_opt(get_num_or(el, u"x2", field_or(*o, u"x2"))));
  o->set(u"z1", from_opt(get_num_or(el, u"z1", field_or(*o, u"z1"))));
  o->set(u"z2", from_opt(get_num_or(el, u"z2", field_or(*o, u"z2"))));
  o->set(u"h1", from_opt(get_num_or(el, u"h1", field_or(*o, u"h1"))));
  o->set(u"h2", from_opt(get_num_or(el, u"h2", field_or(*o, u"h2"))));
  reorder_fields(ret, terrain_info_fields());
  return ret;
}

}
}
}
