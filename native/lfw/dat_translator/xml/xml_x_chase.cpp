#include "lfw/dat_translator/xml/xml_x_chase.h"

#include <memory>
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

// `xml_2_overshoot`：全空 ⇒ undefined；只 1 项 ⇒ {x,y,z} 同值；否则按位置挑非空分量。
Value xml_2_overshoot(const IXMLElement& el) {
  const std::optional<std::vector<std::optional<double>>> over =
      el.nums_attr_soft(u"overshoot");
  bool any = false;
  if (over) {
    for (const std::optional<double>& v : *over) {
      if (v) any = true;
    }
  }
  if (!any) return Value();
  if (over->size() == 1) {
    const double v = *(*over)[0];
    auto o = std::make_shared<Object>();
    o->set(u"x", Value(v));
    o->set(u"y", Value(v));
    o->set(u"z", Value(v));
    return Value(std::move(o));
  }
  auto o = std::make_shared<Object>();
  const char16_t* const keys[3] = {u"x", u"y", u"z"};
  for (size_t i = 0; i < 3 && i < over->size(); ++i) {
    if (const std::optional<double> v = (*over)[i]) o->set(std::u16string(keys[i]), Value(*v));
  }
  return Value(std::move(o));
}

// `xml_x_overshoot`：undefined / 三轴全 undefined ⇒ 不写；三轴相等 ⇒ 单数字；否则逗号串。
void xml_x_overshoot(const std::shared_ptr<IXMLElement>& el, const Value& o) {
  if (!truthy(o)) return;
  const std::optional<double> x = opt_num(field_or(o, u"x"));
  const std::optional<double> y = opt_num(field_or(o, u"y"));
  const std::optional<double> z = opt_num(field_or(o, u"z"));
  if (!x && !y && !z) return;
  if (x && y && z && *x == *y && *y == *z) {
    el->set_attr(u"overshoot", Value(*x));
    return;
  }
  auto arr = std::make_shared<Array>();
  arr->push_back(from_opt(x));
  arr->push_back(from_opt(y));
  arr->push_back(from_opt(z));
  el->set_attr(u"overshoot", Value(std::move(arr)));
}

}

Value xml_2_chase(const IXMLElement& el) {
  Value ret = chase_info_new();
  Object* const o = as_object(ret);
  o->set(u"stratedy", from_opt(get_num_or(el, u"stratedy", field_or(*o, u"stratedy"))));
  o->set(u"flag", from_opt(get_num_or(el, u"flag", field_or(*o, u"flag"))));
  o->set(u"lost", from_opt(get_num_or(el, u"lost", field_or(*o, u"lost"))));
  o->set(u"oy", from_opt(get_num_or(el, u"oy", field_or(*o, u"oy"))));
  o->set(u"overshoot", xml_2_overshoot(el));
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_chase(IXML& xml, const Value& c, const std::u16string& tag) {
  if (!truthy(c)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"stratedy", field_or(c, u"stratedy"));
  ret->set_attr(u"flag", field_or(c, u"flag"));
  ret->set_attr(u"lost", field_or(c, u"lost"));
  ret->set_attr(u"oy", field_or(c, u"oy"));
  xml_x_overshoot(ret, field_or(c, u"overshoot"));
  return ret;
}

}
}
}
