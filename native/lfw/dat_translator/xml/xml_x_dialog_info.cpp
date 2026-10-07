#include "lfw/dat_translator/xml/xml_x_dialog_info.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_2_dialog_info(const IXMLElement& el) {
  Value ret = dialog_info_new();
  Object* const o = as_object(ret);
  o->set(u"type", from_opt(get_str_or(el, u"type", field_or(*o, u"type"))));
  o->set(u"fighter", from_opt(get_str_or(el, u"fighter", field_or(*o, u"fighter"))));
  o->set(u"pause", from_opt(get_bool_or(el, u"pause", field_or(*o, u"pause"))));
  o->set(u"i18n", from_opt(get_str_or(el, u"i18n", field_or(*o, u"i18n"))));
  o->set(u"close_by", from_opt(get_str_or(el, u"close_by", field_or(*o, u"close_by"))));
  o->set(u"hide_stats", from_opt(get_num_or(el, u"hide_stats", field_or(*o, u"hide_stats"))));
  o->set(u"end_test", from_opt(el.get_str_arr(u"end_test")));
  delete_undefined(ret);
  reorder_fields(ret, dialog_info_fields());
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_dialog_info(IXML& xml, const Value& d,
                                               const std::u16string& tag) {
  if (!truthy(d)) return nullptr;
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"type", field_or(d, u"type"));
  el->set_attr(u"fighter", field_or(d, u"fighter"));
  el->set_attr(u"pause", field_or(d, u"pause"));
  el->set_attr(u"i18n", field_or(d, u"i18n"));
  el->set_attr(u"close_by", field_or(d, u"close_by"));
  el->set_attr(u"hide_stats", field_or(d, u"hide_stats"));
  el->set_attr(u"end_test", field_or(d, u"end_test"));
  return el;
}

}
}
}
