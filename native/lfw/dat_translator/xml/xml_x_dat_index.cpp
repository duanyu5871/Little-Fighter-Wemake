#include "lfw/dat_translator/xml/xml_x_dat_index.h"

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_dat_index(IXML& xml, const Value& idx,
                                             const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"id", field_or(idx, u"id"));
  el->set_attr(u"type", field_or(idx, u"type"));
  el->set_attr(u"file", field_or(idx, u"file"));
  el->set_attr(u"hash", field_or(idx, u"hash"));
  el->set_attr(u"alias", field_or(idx, u"alias"));
  el->set_attr(u"groups", field_or(idx, u"groups"));
  el->set_attr(u"skipped", field_or(idx, u"skipped"));
  el->set_attr(u"bot", field_or(idx, u"bot"));
  return el;
}

Value xml_2_dat_index(const IXMLElement& el) {
  Value ret = dat_index_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"type", from_opt(get_str_or(el, u"type", field_or(*o, u"type"))));
  o->set(u"file", from_opt(get_str_or(el, u"file", field_or(*o, u"file"))));
  o->set(u"hash", from_opt(el.get_str(u"hash")));
  o->set(u"alias", from_opt(el.get_str(u"alias")));
  o->set(u"groups", from_opt(el.strs_attr(u"groups")));
  o->set(u"skipped", from_opt(el.get_str(u"skipped")));
  o->set(u"bot", from_opt(el.get_str(u"bot")));
  delete_undefined(ret);
  reorder_fields(ret, dat_index_fields());
  return ret;
}

}
}
}
