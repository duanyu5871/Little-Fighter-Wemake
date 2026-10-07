#include "lfw/dat_translator/xml/xml_x_drink_info.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

// `arr?.[i] ?? ret.x`：软数组有值给值，否则给字段缺省（可能是 undefined ⇒ 交给
// `get_num_or` 的 or 一并按「没有」处理）。
Value soft_at(const std::optional<std::vector<std::optional<double>>>& v, size_t i,
              const Value& or_value) {
  if (v && i < v->size() && v->at(i)) return Value(*v->at(i));
  return or_value;
}

void put_soft_attr(const std::shared_ptr<IXMLElement>& el, const char16_t* attr, const Value& obj,
                   const char16_t* a, const char16_t* b, const char16_t* c) {
  auto arr = std::make_shared<Array>();
  arr->push_back(field_or(obj, a));
  arr->push_back(field_or(obj, b));
  arr->push_back(field_or(obj, c));
  el->set_arr_attr_soft(attr, Value(std::move(arr)));
}

}  // namespace

Value xml_2_drink_info(const IXMLElement& el) {
  Value ret = drink_info_new();
  Object* const o = as_object(ret);
  const std::optional<std::vector<std::optional<double>>> hp_h = el.nums_attr_soft(u"hp_h");
  const std::optional<std::vector<std::optional<double>>> hp_r = el.nums_attr_soft(u"hp_r");
  const std::optional<std::vector<std::optional<double>>> mp_h = el.nums_attr_soft(u"mp_h");
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  o->set(u"hp_h_total",
         from_opt(get_num_or(el, u"hp_h_total", soft_at(hp_h, 0, field_or(*o, u"hp_h_total")))));
  o->set(u"hp_h_value",
         from_opt(get_num_or(el, u"hp_h_value", soft_at(hp_h, 1, field_or(*o, u"hp_h_value")))));
  o->set(u"hp_h_ticks",
         from_opt(get_num_or(el, u"hp_h_ticks", soft_at(hp_h, 2, field_or(*o, u"hp_h_ticks")))));
  o->set(u"hp_r_total",
         from_opt(get_num_or(el, u"hp_r_total", soft_at(hp_r, 0, field_or(*o, u"hp_r_total")))));
  o->set(u"hp_r_value",
         from_opt(get_num_or(el, u"hp_r_value", soft_at(hp_r, 1, field_or(*o, u"hp_r_value")))));
  o->set(u"hp_r_ticks",
         from_opt(get_num_or(el, u"hp_r_ticks", soft_at(hp_r, 2, field_or(*o, u"hp_r_ticks")))));
  o->set(u"mp_h_total",
         from_opt(get_num_or(el, u"mp_h_total", soft_at(mp_h, 0, field_or(*o, u"mp_h_total")))));
  o->set(u"mp_h_value",
         from_opt(get_num_or(el, u"mp_h_value", soft_at(mp_h, 1, field_or(*o, u"mp_h_value")))));
  o->set(u"mp_h_ticks",
         from_opt(get_num_or(el, u"mp_h_ticks", soft_at(mp_h, 2, field_or(*o, u"mp_h_ticks")))));
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_drink_info(IXML& xml, const Value& d,
                                              const std::u16string& tag) {
  if (!truthy(d)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(d, u"id"));
  ret->set_attr(u"name", field_or(d, u"name"));
  put_soft_attr(ret, u"hp_h", d, u"hp_h_total", u"hp_h_value", u"hp_h_ticks");
  put_soft_attr(ret, u"hp_r", d, u"hp_r_total", u"hp_r_value", u"hp_r_ticks");
  put_soft_attr(ret, u"mp_h", d, u"mp_h_total", u"mp_h_value", u"mp_h_ticks");
  return ret;
}

}
}
}
