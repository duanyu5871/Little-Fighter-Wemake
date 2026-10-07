#include "lfw/dat_translator/xml/xml_x_frame_model.h"

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

// 与 `xml_x_model_info.cpp` 的 `vec3_of` 相同形状（TS 里也是复制的两段）。
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

Value xml_2_frame_model(const IXMLElement* el) {
  if (el == nullptr) return Value();
  Value ret = frame_model_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(*el, u"id", field_or(*o, u"id"))));
  o->set(u"anim", from_opt(el->get_str(u"anim")));
  o->set(u"seek", from_opt(el->get_num(u"seek")));
  o->set(u"loop", from_opt(el->get_bool(u"loop")));
  o->set(u"time_scale", from_opt(el->get_num(u"time_scale")));
  o->set(u"reverse", from_opt(el->get_bool(u"reverse")));
  o->set(u"rad", from_opt(el->get_num(u"rad")));
  {
    const Value rotation = vec3_attr(*el, u"rotation");
    if (truthy(rotation)) o->set(u"rotation", rotation);
  }
  {
    const Value scale = vec3_attr(*el, u"scale");
    if (truthy(scale)) o->set(u"scale", scale);
  }
  {
    const Value offset = vec3_attr(*el, u"offset");
    if (truthy(offset)) o->set(u"offset", offset);
  }
  const IXMLElement* const pose_el = el->child_by_tag(u"pose");
  if (pose_el != nullptr) {
    auto pose = std::make_shared<Object>();
    const std::optional<std::vector<std::u16string>> bones = pose_el->get_str_arr(u"bones");
    if (bones && !bones->empty()) pose->set(u"bones", from_opt(bones));
    const std::optional<std::vector<double>> pos = pose_el->get_num_arr(u"pos");
    if (pos && !pos->empty()) pose->set(u"pos", from_opt(pos));
    const std::optional<std::vector<double>> rot = pose_el->get_num_arr(u"rot");
    if (rot && !rot->empty()) pose->set(u"rot", from_opt(rot));
    const std::optional<std::vector<double>> scl = pose_el->get_num_arr(u"scl");
    if (scl && !scl->empty()) pose->set(u"scl", from_opt(scl));
    o->set(u"pose", Value(std::move(pose)));
  }
  return delete_undefined(ret);
}

std::shared_ptr<IXMLElement> xml_x_frame_model(IXML& xml, const Value& m,
                                               const std::u16string& tag) {
  if (!truthy(m)) return nullptr;
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(m, u"id"));
  ret->set_attr(u"anim", field_or(m, u"anim"));
  ret->set_attr(u"seek", field_or(m, u"seek"));
  ret->set_attr(u"loop", field_or(m, u"loop"));
  ret->set_attr(u"time_scale", field_or(m, u"time_scale"));
  ret->set_attr(u"reverse", field_or(m, u"reverse"));
  ret->set_attr(u"rad", field_or(m, u"rad"));
  const Value rotation = field_or(m, u"rotation");
  if (truthy(rotation)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(rotation, u"x"));
    arr->push_back(field_or(rotation, u"y"));
    arr->push_back(field_or(rotation, u"z"));
    ret->set_arr_attr_soft(u"rotation", Value(std::move(arr)));
  }
  const Value scale = field_or(m, u"scale");
  if (truthy(scale)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(scale, u"x"));
    arr->push_back(field_or(scale, u"y"));
    arr->push_back(field_or(scale, u"z"));
    ret->set_arr_attr_soft(u"scale", Value(std::move(arr)));
  }
  const Value offset = field_or(m, u"offset");
  if (truthy(offset)) {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(offset, u"x"));
    arr->push_back(field_or(offset, u"y"));
    arr->push_back(field_or(offset, u"z"));
    ret->set_arr_attr_soft(u"offset", Value(std::move(arr)));
  }
  const Value pose = field_or(m, u"pose");
  if (truthy(pose)) {
    std::shared_ptr<IXMLElement> pe = xml.create(u"pose");
    pe->set_attr(u"bones", field_or(pose, u"bones"));
    pe->set_attr(u"pos", field_or(pose, u"pos"));
    pe->set_attr(u"rot", field_or(pose, u"rot"));
    pe->set_attr(u"scl", field_or(pose, u"scl"));
    ret->insert(pe);
  }
  return ret;
}

}
}
}
