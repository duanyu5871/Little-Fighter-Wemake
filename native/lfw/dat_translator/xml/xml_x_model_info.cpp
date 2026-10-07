#include "lfw/dat_translator/xml/xml_x_model_info.h"

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_map.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

// `nums_attr_soft(...)` 的 `.some(v => v != null)` + 只收有值项的子对象。
Value vec3_of(const std::optional<std::vector<std::optional<double>>>& v) {
  if (!v) return Value();
  bool any = false;
  for (const std::optional<double>& item : *v) {
    if (item) {
      any = true;
      break;
    }
  }
  if (!any) return Value();
  auto out = std::make_shared<Object>();
  static const char16_t* const kKeys[3] = {u"x", u"y", u"z"};
  for (size_t i = 0; i < 3; ++i) {
    if (i < v->size() && v->at(i)) out->set(kKeys[i], Value(*v->at(i)));
  }
  return Value(std::move(out));
}

Value vec3_attr(const IXMLElement& el, const char16_t* name) {
  return vec3_of(el.nums_attr_soft(name));
}

}  // namespace

Value xml_2_model_info(const IXMLElement& el) {
  Value ret = model_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"path", from_opt(get_str_or(el, u"path", field_or(*o, u"path"))));
  o->set(u"variants", from_opt(el.get_str_arr(u"variants")));
  const Value scale = vec3_attr(el, u"scale");
  if (truthy(scale)) o->set(u"scale", scale);
  const Value offset = vec3_attr(el, u"offset");
  if (truthy(offset)) o->set(u"offset", offset);
  const Value rotation = vec3_attr(el, u"rotation");
  if (truthy(rotation)) o->set(u"rotation", rotation);
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_model_info(IXML& xml, const Value& f,
                                              const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(f, u"id"));
  ret->set_attr(u"path", field_or(f, u"path"));
  ret->set_attr(u"variants", field_join(field_or(f, u"variants")));
  const Value rotation = field_or(f, u"rotation");
  if (truthy(rotation)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(rotation, u"x"));
    arr->push_back(field_or(rotation, u"y"));
    arr->push_back(field_or(rotation, u"z"));
    ret->set_arr_attr_soft(u"rotation", Value(std::move(arr)));
  }
  const Value scale = field_or(f, u"scale");
  if (truthy(scale)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(scale, u"x"));
    arr->push_back(field_or(scale, u"y"));
    arr->push_back(field_or(scale, u"z"));
    ret->set_arr_attr_soft(u"scale", Value(std::move(arr)));
  }
  const Value offset = field_or(f, u"offset");
  if (truthy(offset)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(offset, u"x"));
    arr->push_back(field_or(offset, u"y"));
    arr->push_back(field_or(offset, u"z"));
    ret->set_arr_attr_soft(u"offset", Value(std::move(arr)));
  }
  return ret;
}

Value xml_2_model_info_map(const IXMLElement& el, const std::u16string& tag) {
  return xml_2_map(el, {tag}, xml_2_model_info);
}

std::optional<std::vector<std::shared_ptr<IXMLElement>>> xml_x_model_info_map(
    IXML& xml, const Value& map, const std::u16string& tag) {
  return xml_x_map(xml, map, tag, xml_x_model_info);
}

}
}
}
