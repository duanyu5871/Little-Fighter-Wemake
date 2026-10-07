#include "lfw/dat_translator/xml/xml_x_bg_layer.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

Value arr_at(const std::optional<std::vector<double>>& v, size_t i, const Value& or_value) {
  if (v && i < v->size()) return Value(v->at(i));
  return or_value;
}

}  // namespace

Value xml_2_bg_layer(const IXMLElement& el, size_t index) {
  Value ret = bg_layer_info_new();
  Object* const o = as_object(ret);
  const std::optional<std::vector<double>> pos = el.get_num_arr(u"pos");
  const std::optional<std::vector<double>> size = el.get_num_arr(u"size");
  const std::optional<std::vector<double>> rect = el.get_num_arr(u"rect");
  const std::optional<std::vector<double>> offsetAnim = el.get_num_arr(u"offsetAnim");

  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  o->set(u"file", from_opt(get_str_or(el, u"file", field_or(*o, u"file"))));
  o->set(u"width", from_opt(get_num_or(el, u"width", field_or(*o, u"width"))));
  o->set(u"height", from_opt(get_num_or(el, u"height", field_or(*o, u"height"))));
  o->set(u"x", from_opt(get_num_or(el, u"x", arr_at(pos, 0, arr_at(rect, 0, field_or(*o, u"x"))))));
  o->set(u"y", from_opt(get_num_or(el, u"y", arr_at(pos, 1, arr_at(rect, 1, field_or(*o, u"y"))))));
  o->set(u"w", from_opt(get_num_or(el, u"w", arr_at(size, 0, arr_at(rect, 2, field_or(*o, u"w"))))));
  o->set(u"h", from_opt(get_num_or(el, u"h", arr_at(size, 1, arr_at(rect, 3, field_or(*o, u"h"))))));
  o->set(u"dw", from_opt(get_num_or(el, u"dw", field_or(*o, u"dw"))));
  o->set(u"dh", from_opt(get_num_or(el, u"dh", field_or(*o, u"dh"))));
  o->set(u"uv_loop", from_opt(get_num_or(el, u"uv_loop", field_or(*o, u"uv_loop"))));
  o->set(u"z", from_opt(get_num_or(el, u"z", Value(static_cast<double>(index)))));
  o->set(u"loop", from_opt(get_num_or(el, u"loop", field_or(*o, u"loop"))));
  o->set(u"absolute", from_opt(get_num_or(el, u"absolute", field_or(*o, u"absolute"))));
  o->set(u"color", from_opt(get_str_or(el, u"color", field_or(*o, u"color"))));
  o->set(u"opacity", from_opt(get_num_or(el, u"opacity", field_or(*o, u"opacity"))));
  o->set(u"cc", from_opt(get_num_or(el, u"cc", field_or(*o, u"cc"))));
  o->set(u"c1", from_opt(get_num_or(el, u"c1", field_or(*o, u"c1"))));
  o->set(u"c2", from_opt(get_num_or(el, u"c2", field_or(*o, u"c2"))));
  o->set(u"offsetAnimX",
         from_opt(get_num_or(el, u"offsetAnimX", arr_at(offsetAnim, 0, field_or(*o, u"offsetAnimX")))));
  o->set(u"offsetAnimY",
         from_opt(get_num_or(el, u"offsetAnimY", arr_at(offsetAnim, 1, field_or(*o, u"offsetAnimY")))));
  delete_undefined(ret);
  reorder_fields(ret, bg_layer_info_fields());
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_bg_layer(IXML& xml, const Value& l,
                                            const std::u16string& tag) {
  std::shared_ptr<IXMLElement> layer = xml.create(tag);
  layer->set_attr(u"width", field_or(l, u"width"));
  layer->set_attr(u"height", field_or(l, u"height"));
  layer->set_attr(u"x", field_or(l, u"x"));
  layer->set_attr(u"y", field_or(l, u"y"));
  layer->set_attr(u"z", field_or(l, u"z"));
  layer->set_attr(u"w", field_or(l, u"w"));
  layer->set_attr(u"h", field_or(l, u"h"));
  layer->set_attr(u"dw", field_or(l, u"dw"));
  layer->set_attr(u"dh", field_or(l, u"dh"));
  layer->set_attr(u"uv_loop", field_or(l, u"uv_loop"));
  layer->set_attr(u"loop", field_or(l, u"loop"));
  layer->set_attr(u"absolute", field_or(l, u"absolute"));
  layer->set_attr(u"cc", field_or(l, u"cc"));
  layer->set_attr(u"c1", field_or(l, u"c1"));
  layer->set_attr(u"c2", field_or(l, u"c2"));
  layer->set_attr(u"offsetAnimX", field_or(l, u"offsetAnimX"));
  layer->set_attr(u"offsetAnimY", field_or(l, u"offsetAnimY"));
  layer->set_attr(u"id", field_or(l, u"id"));
  layer->set_attr(u"name", field_or(l, u"name"));
  layer->set_attr(u"file", field_or(l, u"file"));
  layer->set_attr(u"color", field_or(l, u"color"));
  layer->set_attr(u"opacity", field_or(l, u"opacity"));
  return layer;
}

}
}
}
