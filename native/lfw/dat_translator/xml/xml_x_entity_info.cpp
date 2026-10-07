#include "lfw/dat_translator/xml/xml_x_entity_info.h"

#include <optional>
#include <vector>

#include "lfw/dat_translator/xml/xml_from_world_dataset.h"
#include "lfw/dat_translator/xml/xml_to_world_dataset.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_armor_info.h"
#include "lfw/dat_translator/xml/xml_x_drink_info.h"
#include "lfw/dat_translator/xml/xml_x_frame_pic.h"
#include "lfw/dat_translator/xml/xml_x_map.h"
#include "lfw/dat_translator/xml/xml_x_model_info.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_opoint.h"
#include "lfw/dat_translator/xml/xml_x_picture_info.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

Value soft_at(const std::optional<std::vector<std::optional<double>>>& v, size_t i,
              const Value& or_value) {
  if (v && i < v->size() && v->at(i)) return Value(*v->at(i));
  return or_value;
}

// 内联 models 循环里的 `{ x: scale[0], y: scale[1], z: scale[2] }` —— 逐项直取
// （每项都可能有值 undefined，端口按 JS 对象原样装）。
void set_vec3(Object& target, const char16_t* key,
              const std::optional<std::vector<std::optional<double>>>& v, size_t count) {
  bool any = false;
  for (const std::optional<double>& item : *v) {
    if (item) {
      any = true;
      break;
    }
  }
  if (!any) return;
  auto out = std::make_shared<Object>();
  static const char16_t* const kKeys[4] = {u"x", u"y", u"z", u"w"};
  for (size_t i = 0; i < count; ++i) {
    out->set(kKeys[i], i < v->size() && v->at(i) ? Value(*v->at(i)) : Value());
  }
  target.set(key, Value(std::move(out)));
}

}  // namespace

std::shared_ptr<IXMLElement> xml_x_entity_info(IXML& xml, const Value& info,
                                               const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"type", field_or(info, u"type"));
  ret->set_attr(u"name", field_or(info, u"name"));
  ret->set_attr(u"head", field_or(info, u"head"));
  ret->set_attr(u"small", field_or(info, u"small"));
  ret->set_attr(u"ce", field_or(info, u"ce"));
  ret->set_attr(u"weight", field_or(info, u"weight"));
  ret->set_attr(u"group", field_join(field_or(info, u"group")));
  {
    const std::optional<std::vector<std::shared_ptr<IXMLElement>>> files =
        xml_x_picture_info_map(xml, field_or(info, u"files"), u"file");
    if (files) {
      for (const std::shared_ptr<IXMLElement>& v : *files) ret->insert(v);
    }
  }
  {
    const std::optional<std::vector<std::shared_ptr<IXMLElement>>> models =
        xml_x_model_info_map(xml, field_or(info, u"models"), u"model");
    if (models) {
      for (const std::shared_ptr<IXMLElement>& v : *models) ret->insert(v);
    }
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(info, u"bounce_x"));
    arr->push_back(field_or(info, u"bounce_y"));
    arr->push_back(field_or(info, u"bounce_z"));
    ret->set_arr_attr_soft(u"bounce", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(info, u"bounce_min_x"));
    arr->push_back(field_or(info, u"bounce_min_y"));
    arr->push_back(field_or(info, u"bounce_min_z"));
    ret->set_arr_attr_soft(u"bounce_min", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(info, u"fast_vx"));
    arr->push_back(field_or(info, u"fast_vy"));
    arr->push_back(field_or(info, u"fast_vz"));
    ret->set_arr_attr_soft(u"fast", Value(std::move(arr)));
  }
  xml_x_non_empty(xml, field_or(info, u"brokens"), u"broken", xml_x_opoint, ret);
  ret->insert(xml_x_armor_info(xml, field_or(info, u"armor"), u"armor"));
  ret->insert(xml_x_drink_info(xml, field_or(info, u"drink"), u"drink"));
  ret->set_attr(u"drop_hurt", field_or(info, u"drop_hurt"));
  ret->set_attr(u"hit_sounds", field_or(info, u"hit_sounds"));
  ret->set_attr(u"drop_sounds", field_or(info, u"drop_sounds"));
  ret->set_attr(u"dead_sounds", field_or(info, u"dead_sounds"));
  ret->set_attr(u"bot_id", field_or(info, u"bot_id"));
  ret->set_attr(u"bot_ignore", field_or(info, u"bot_ignore"));
  ret->set_attr(u"w_atk_m_x", field_or(info, u"w_atk_m_x"));
  ret->set_attr(u"w_atk_r_x", field_or(info, u"w_atk_r_x"));
  xml_x_map(xml, field_or(info, u"portraits"), u"portrait", xml_x_frame_pic, ret);
  ret->insert(xml_from_world_dataset(xml, info, u"dataset"));
  return ret;
}

Value xml_2_entity_info(const IXMLElement& el) {
  Value ret = entity_info_new();
  Object* const o = as_object(ret);

  o->set(u"type", from_opt(get_num_or(el, u"type", field_or(*o, u"type"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  o->set(u"head", from_opt(get_str_or(el, u"head", field_or(*o, u"head"))));
  o->set(u"small", from_opt(get_str_or(el, u"small", field_or(*o, u"small"))));
  o->set(u"ce", from_opt(get_num_or(el, u"ce", field_or(*o, u"ce"))));
  o->set(u"weight", from_opt(get_num_or(el, u"weight", field_or(*o, u"weight"))));
  o->set(u"group", from_opt(el.get_str_arr(u"group")));
  o->set(u"files", xml_2_picture_info_map(el, u"file"));
  o->set(u"models", xml_2_model_info_map(el, u"model"));

  {
    const std::optional<std::vector<std::optional<double>>> bounce = el.nums_attr_soft(u"bounce");
    o->set(u"bounce_x", from_opt(get_num_or(el, u"bounce_x", soft_at(bounce, 0, field_or(*o, u"bounce_x")))));
    o->set(u"bounce_y", from_opt(get_num_or(el, u"bounce_y", soft_at(bounce, 1, field_or(*o, u"bounce_y")))));
    o->set(u"bounce_z", from_opt(get_num_or(el, u"bounce_z", soft_at(bounce, 2, field_or(*o, u"bounce_z")))));
  }
  {
    // ⚠ TS 读向把软数组分给了 y、x、z（写向是 x、y、z）。
    const std::optional<std::vector<std::optional<double>>> bounce_min =
        el.nums_attr_soft(u"bounce_min");
    o->set(u"bounce_min_y", from_opt(get_num_or(el, u"bounce_min_y", soft_at(bounce_min, 0, field_or(*o, u"bounce_min_y")))));
    o->set(u"bounce_min_x", from_opt(get_num_or(el, u"bounce_min_x", soft_at(bounce_min, 1, field_or(*o, u"bounce_min_x")))));
    o->set(u"bounce_min_z", from_opt(get_num_or(el, u"bounce_min_z", soft_at(bounce_min, 2, field_or(*o, u"bounce_min_z")))));
  }
  {
    // ⚠ TS 读向读的是 `fast_v`（写向写的是 `fast`），且 [0]→vy、[1]→vx。
    const std::optional<std::vector<std::optional<double>>> fast_v = el.nums_attr_soft(u"fast_v");
    o->set(u"fast_vy", from_opt(get_num_or(el, u"fast_vy", soft_at(fast_v, 0, field_or(*o, u"fast_vy")))));
    o->set(u"fast_vx", from_opt(get_num_or(el, u"fast_vx", soft_at(fast_v, 1, field_or(*o, u"fast_vx")))));
    o->set(u"fast_vz", from_opt(get_num_or(el, u"fast_vz", soft_at(fast_v, 2, field_or(*o, u"fast_vz")))));
  }

  o->set(u"drop_hurt", from_opt(el.get_num(u"drop_hurt")));
  o->set(u"hit_sounds", from_opt(el.get_str_arr(u"hit_sounds")));
  o->set(u"drop_sounds", from_opt(el.get_str_arr(u"drop_sounds")));
  o->set(u"dead_sounds", from_opt(el.get_str_arr(u"dead_sounds")));

  {
    // `el.get_str("bot_id") ?? el.child_by_tag("bot")?.get_str("id")`
    const IXMLElement* const bot = el.child_by_tag(u"bot");
    const Value or_value = bot != nullptr ? from_opt(bot->get_str(u"id")) : Value();
    o->set(u"bot_id", from_opt(get_str_or(el, u"bot_id", or_value)));
  }
  o->set(u"bot_ignore", from_opt(get_num_or(el, u"bot_ignore", field_or(*o, u"bot_ignore"))));
  o->set(u"w_atk_m_x", from_opt(get_num_or(el, u"w_atk_m_x", field_or(*o, u"w_atk_m_x"))));
  o->set(u"w_atk_r_x", from_opt(get_num_or(el, u"w_atk_r_x", field_or(*o, u"w_atk_r_x"))));

  {
    // portraits：内联手写循环（name 缺省空串，值缺省 tex "0" / 坐标 0）。
    auto portraits = std::make_shared<Object>();
    for (IXMLElement* p : el.children_by_tag(u"portrait")) {
      const std::u16string name = p->get_str(u"name").value_or(u"");
      auto item = std::make_shared<Object>();
      item->set(u"tex", Value(p->get_str(u"tex").value_or(u"0")));
      const std::optional<double> x = p->get_num(u"x");
      item->set(u"x", Value(x ? *x : 0));
      const std::optional<double> y = p->get_num(u"y");
      item->set(u"y", Value(y ? *y : 0));
      const std::optional<double> w = p->get_num(u"w");
      item->set(u"w", Value(w ? *w : 0));
      const std::optional<double> h = p->get_num(u"h");
      item->set(u"h", Value(h ? *h : 0));
      portraits->set(name, Value(std::move(item)));
    }
    if (!portraits->keys().empty()) o->set(u"portraits", Value(std::move(portraits)));
  }

  {
    const IXMLElement* const drink_el = el.child_by_tag(u"drink");
    if (drink_el != nullptr) o->set(u"drink", xml_2_drink_info(*drink_el));
    const IXMLElement* const armor_el = el.child_by_tag(u"armor");
    if (armor_el != nullptr) o->set(u"armor", xml_2_armor_info(armor_el));
  }

  {
    // models：内联手写循环（name 取 name→id→空串；variants 是 `strs_attr` ⇒ 缺了 undefined）。
    auto models = std::make_shared<Object>();
    for (IXMLElement* m : el.children_by_tag(u"model")) {
      std::optional<std::u16string> name = m->get_str(u"name");
      if (!name) name = m->get_str(u"id");
      const std::u16string name_v = name.value_or(u"");
      auto model = std::make_shared<Object>();
      const std::optional<std::u16string> id = m->get_str(u"id");
      model->set(u"id", Value(id ? *id : name_v));
      model->set(u"path", Value(m->get_str(u"path").value_or(u"")));
      model->set(u"variants", from_opt(m->strs_attr(u"variants")));
      const std::optional<std::vector<std::optional<double>>> scale = m->nums_attr_soft(u"scale");
      if (scale) set_vec3(*model, u"scale", scale, 3);
      const std::optional<std::vector<std::optional<double>>> offset =
          m->nums_attr_soft(u"offset");
      if (offset) set_vec3(*model, u"offset", offset, 3);
      const std::optional<std::vector<std::optional<double>>> quat =
          m->nums_attr_soft(u"quaternion");
      if (quat) set_vec3(*model, u"quaternion", quat, 4);
      models->set(name_v, Value(std::move(model)));
    }
    if (!models->keys().empty()) o->set(u"models", Value(std::move(models)));
  }

  {
    auto brokens = std::make_shared<Array>();
    for (IXMLElement* v : el.children_by_tag(u"broken")) brokens->push_back(xml_2_opoint(*v));
    for (IXMLElement* v : el.children_by_tag(u"opoint")) brokens->push_back(xml_2_opoint(*v));
    if (brokens->size() != 0) o->set(u"brokens", Value(std::move(brokens)));
  }

  {
    // dataset 的键逐个盖回顶层（undefined 也盖过去）。
    const Value ds = xml_to_world_dataset(el.child_by_tag(u"dataset"));
    const Object* const dso = as_object(ds);
    if (dso != nullptr) {
      for (const std::u16string& k : dso->keys()) {
        const Value* const v = dso->get(k);
        o->set(k, v != nullptr ? *v : Value());
      }
    }
  }

  return ret;
}

}
}
}
