#include "lfw/dat_translator/xml/xml_x_bg_info.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/defines_data.h"
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

Value xml_2_bg_info(const IXMLElement& el) {
  Value ret = bg_info_new();
  Object* const o = as_object(ret);
  const std::optional<std::vector<double>> bound = el.get_num_arr(u"bound");
  const std::optional<std::vector<double>> zoom = el.get_num_arr(u"zoom");
  const std::optional<std::vector<double>> shadowsize = el.get_num_arr(u"shadowsize");
  o->set(u"name", from_opt(el.get_str(u"name")));
  {
    const Value shadow = field_or(*o, u"shadow");
    o->set(u"shadow", from_opt(get_str_or(el, u"shadow", shadow)));
  }
  {
    // `el.get_str_arr("group") ?? ["regular"]`
    const std::optional<std::vector<std::u16string>> group = el.get_str_arr(u"group");
    if (group) {
      o->set(u"group", from_opt(group));
    } else {
      auto arr = std::make_shared<Array>();
      arr->push_back(Value(u"regular"));
      o->set(u"group", Value(std::move(arr)));
    }
  }
  o->set(u"left",
         from_opt(get_num_or(el, u"left", arr_at(bound, 0, field_or(*o, u"left")))));
  o->set(u"right",
         from_opt(get_num_or(el, u"right", arr_at(bound, 1, field_or(*o, u"right")))));
  o->set(u"far", from_opt(get_num_or(el, u"far", arr_at(bound, 2, field_or(*o, u"far")))));
  o->set(u"near", from_opt(get_num_or(el, u"near", arr_at(bound, 3, field_or(*o, u"near")))));
  {
    const std::optional<double> height = el.get_num(u"height");
    o->set(u"height", Value(height ? *height : defines::num(u"Defines.MODERN_SCREEN_HEIGHT")));
  }
  o->set(u"shadow_w",
         from_opt(get_num_or(el, u"shadow_w", arr_at(shadowsize, 0, field_or(*o, u"shadow_w")))));
  o->set(u"shadow_h",
         from_opt(get_num_or(el, u"shadow_h", arr_at(shadowsize, 1, field_or(*o, u"shadow_h")))));
  o->set(u"zoom_x", from_opt(get_num_or(el, u"zoom_x", arr_at(zoom, 0, field_or(*o, u"zoom_x")))));
  o->set(u"zoom_y", from_opt(get_num_or(el, u"zoom_y", arr_at(zoom, 1, field_or(*o, u"zoom_y")))));
  o->set(u"zoom_z", from_opt(get_num_or(el, u"zoom_z", arr_at(zoom, 2, field_or(*o, u"zoom_z")))));
  delete_undefined(ret);
  reorder_fields(ret, bg_info_fields());
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_bg_info(IXML& xml, const Value& b, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> info = xml.create(tag);
  info->set_attr(u"name", field_or(b, u"name"));
  info->set_attr(u"shadow", field_or(b, u"shadow"));
  {
    // `b.group?.join() || void 0`
    const Value joined = field_join(field_or(b, u"group"));
    info->set_attr(u"group", truthy(joined) ? joined : Value());
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(b, u"left"));
    arr->push_back(field_or(b, u"right"));
    arr->push_back(field_or(b, u"far"));
    arr->push_back(field_or(b, u"near"));
    info->set_arr_attr_soft(u"bound", Value(std::move(arr)));
  }
  info->set_attr(u"height", field_or(b, u"height"));
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(b, u"shadow_w"));
    arr->push_back(field_or(b, u"shadow_h"));
    info->set_arr_attr_soft(u"shadowsize", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(b, u"zoom_x"));
    arr->push_back(field_or(b, u"zoom_y"));
    arr->push_back(field_or(b, u"zoom_z"));
    info->set_arr_attr_soft(u"zoom", Value(std::move(arr)));
  }
  return info;
}

}
}
}
