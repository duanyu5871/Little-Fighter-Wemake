#include "lfw/dat_translator/xml/xml_x_frame_pic.h"

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_map.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

// `rect?.[i] ?? ret.x`：软数组有值给值，否则给字段缺省（可能是 undefined ⇒ 交给
// `get_num_or` 的 or 一并按「没有」处理）。
Value rect_at(const std::optional<std::vector<double>>& rect, size_t i, const Value& or_value) {
  if (rect && i < rect->size()) return Value(rect->at(i));
  return or_value;
}

}  // namespace

Value xml_2_frame_pic(const IXMLElement& el) {
  Value ret = frame_pic_new();
  Object* const o = as_object(ret);
  o->set(u"tex", from_opt(get_str_or(el, u"tex", field_or(*o, u"tex"))));
  const std::optional<std::vector<double>> rect = el.get_num_arr(u"rect");
  o->set(u"x", from_opt(get_num_or(el, u"x", rect_at(rect, 0, field_or(*o, u"x")))));
  o->set(u"y", from_opt(get_num_or(el, u"y", rect_at(rect, 1, field_or(*o, u"y")))));
  o->set(u"w", from_opt(get_num_or(el, u"w", rect_at(rect, 2, field_or(*o, u"w")))));
  o->set(u"h", from_opt(get_num_or(el, u"h", rect_at(rect, 3, field_or(*o, u"h")))));
  o->set(u"deg", from_opt(el.get_num(u"deg")));
  o->set(u"rad", from_opt(el.get_num(u"rad")));
  o->set(u"ox", from_opt(el.get_num(u"ox")));
  o->set(u"oy", from_opt(el.get_num(u"oy")));
  o->set(u"cx", from_opt(el.get_num(u"cx")));
  o->set(u"cy", from_opt(el.get_num(u"cy")));
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_frame_pic(IXML& xml, const Value& pic,
                                             const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"tex", field_or(pic, u"tex"));
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(pic, u"x"));
    arr->push_back(field_or(pic, u"y"));
    arr->push_back(field_or(pic, u"w"));
    arr->push_back(field_or(pic, u"h"));
    el->set_attr(u"rect", Value(std::move(arr)));
  }
  el->set_attr(u"deg", field_or(pic, u"deg"));
  el->set_attr(u"rad", field_or(pic, u"rad"));
  el->set_attr(u"ox", field_or(pic, u"ox"));
  el->set_attr(u"oy", field_or(pic, u"oy"));
  el->set_attr(u"cx", field_or(pic, u"cx"));
  el->set_attr(u"cy", field_or(pic, u"cy"));
  return el;
}

Value xml_2_frame_pic_map(const IXMLElement& el, const std::u16string& tag) {
  return xml_2_map(el, {tag}, xml_2_frame_pic);
}

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_frame_pic_map(
    IXML& xml, const Value& map, const std::u16string& tag) {
  return xml_x_map(xml, map, tag, xml_x_frame_pic);
}

}
}
}
