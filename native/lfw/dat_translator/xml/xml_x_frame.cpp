#include "lfw/dat_translator/xml/xml_x_frame.h"

#include <optional>
#include <variant>
#include <vector>

#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/merge_by_tag.h"
#include "lfw/dat_translator/xml/one_or_arr.h"
#include "lfw/dat_translator/xml/xml_to_velocity_info.h"
#include "lfw/dat_translator/xml/xml_to_world_dataset.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_bdy.h"
#include "lfw/dat_translator/xml/xml_x_bpoint.h"
#include "lfw/dat_translator/xml/xml_x_chase.h"
#include "lfw/dat_translator/xml/xml_x_cpoint.h"
#include "lfw/dat_translator/xml/xml_x_frame_model.h"
#include "lfw/dat_translator/xml/xml_x_frame_pic.h"
#include "lfw/dat_translator/xml/xml_x_hit_key_map.h"
#include "lfw/dat_translator/xml/xml_x_itr.h"
#include "lfw/dat_translator/xml/xml_x_next_frame.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_opoint.h"
#include "lfw/dat_translator/xml/xml_x_wpoint.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

void insert_each(const std::shared_ptr<IXMLElement>& target,
                 const std::optional<std::vector<std::shared_ptr<IXMLElement>>>& els) {
  if (!els) return;
  for (const std::shared_ptr<IXMLElement>& el : *els) target->insert(el);
}

// `FRAME_BEHAVIOR_LABEL_MAP[f.behavior]` / `StateEnumNames[f.state]`：键按
// `String(key)` 查（JS 对象下标就是字符串键），假值不写。
void set_label_attr(const std::shared_ptr<IXMLElement>& el, const char16_t* table_name,
                    const char16_t* attr, const Value& key) {
  if (is_nullish(key)) return;
  const Value* const table = defines::find(std::u16string(table_name));
  const Object* const t = table != nullptr ? as_object(*table) : nullptr;
  if (t == nullptr) return;
  const Value* const label = t->get(to_string(key));
  if (label == nullptr || !truthy(*label)) return;
  el->set_attr(std::u16string(attr), *label);
}

Value arr_at(const std::optional<std::vector<double>>& v, size_t i, const Value& or_value) {
  if (v && i < v->size()) return Value(v->at(i));
  return or_value;
}

}  // namespace

std::shared_ptr<IXMLElement> xml_x_frame(IXML& xml, const Value& f, const std::u16string& tag) {
  std::shared_ptr<IXMLElement> ret = xml.create(tag);
  ret->set_attr(u"id", field_or(f, u"id"));
  ret->set_attr(u"name", field_or(f, u"name"));
  ret->set_attr(u"ref", field_or(f, u"ref"));
  {
    const Value pic = field_or(f, u"pic");
    if (truthy(pic)) ret->insert(xml_x_frame_pic(xml, pic, u"pic"));
  }
  if (const Array* const pics = as_array(field_or(f, u"pics"))) {
    for (size_t i = 0; i < pics->size(); ++i) {
      ret->insert(xml_x_frame_pic(xml, pics->at(i), u"pic"));
    }
  }
  {
    std::shared_ptr<IXMLElement> model_el = xml_x_frame_model(xml, field_or(f, u"model"), u"model");
    if (model_el) ret->insert(model_el);
  }
  ret->set_attr(u"state", field_or(f, u"state"));
  ret->set_attr(u"wait", field_or(f, u"wait"));

  {
    const std::vector<std::shared_ptr<IXMLElement>> next =
        xml_x_t_next_frame(xml, field_or(f, u"next"), u"next");
    for (const std::shared_ptr<IXMLElement>& v : next) ret->insert(v);
  }

  {
    // `[f.centerx, f.centery].join()` —— 缺值就是空段（全缺 ⇒ `","`）。
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(f, u"centerx"));
    arr->push_back(field_or(f, u"centery"));
    ret->set_attr(u"center", Value(std::move(arr)));
  }
  {
    auto arr = std::make_shared<Array>();
    arr->push_back(field_or(f, u"width"));
    arr->push_back(field_or(f, u"height"));
    ret->set_attr(u"size", Value(std::move(arr)));
  }

  {
    // `const sounds = Array.isArray(f.sound) ? f.sound : f.sound ? [f.sound] : void 0;`
    const Value sound = field_or(f, u"sound");
    const Array* const sounds = as_array(sound);
    if (sounds != nullptr) {
      for (size_t i = 0; i < sounds->size(); ++i) {
        std::shared_ptr<IXMLElement> el = xml.create(u"sound");
        el->set_attr(u"value", sounds->at(i));
        ret->insert(el);
      }
    } else if (truthy(sound)) {
      std::shared_ptr<IXMLElement> el = xml.create(u"sound");
      el->set_attr(u"value", sound);
      ret->insert(el);
    }
  }
  ret->set_attr(u"hp", field_or(f, u"hp"));
  ret->set_attr(u"mp", field_or(f, u"mp"));

  ret->set_attr(u"invisible", field_or(f, u"invisible"));
  ret->set_attr(u"invulnerable", field_or(f, u"invulnerable"));
  ret->set_attr(u"blinking", field_or(f, u"blinking"));
  ret->set_attr(u"bot_ignore", field_or(f, u"bot_ignore"));
  ret->set_attr(u"no_shadow", field_or(f, u"no_shadow"));
  ret->set_attr(u"jump_flag", field_or(f, u"jump_flag"));
  ret->set_attr(u"behavior", field_or(f, u"behavior"));
  ret->set_attr(u"landable", field_or(f, u"landable"));
  ret->set_attr(u"facing", field_or(f, u"facing"));
  ret->set_attr(u"stat_recover", field_or(f, u"stat_recover"));
  ret->set_attr(u"toughness_recover", field_or(f, u"toughness_recover"));

  insert_each(ret, xml_x_hit_key_map(xml, field_or(f, u"hit"), u"hit"));
  insert_each(ret, xml_x_hit_key_map(xml, field_or(f, u"hold"), u"hold"));
  insert_each(ret, xml_x_hit_key_map(xml, field_or(f, u"key_down"), u"key_down"));
  insert_each(ret, xml_x_hit_key_map(xml, field_or(f, u"key_up"), u"key_up"));

  if (const Array* const bdy = as_array(field_or(f, u"bdy"))) {
    for (size_t i = 0; i < bdy->size(); ++i) ret->insert(xml_x_bdy(xml, bdy->at(i), u"bdy"));
  }
  if (const Array* const itr = as_array(field_or(f, u"itr"))) {
    for (size_t i = 0; i < itr->size(); ++i) ret->insert(xml_x_itr(xml, itr->at(i), u"itr"));
  }
  if (const Array* const opoint = as_array(field_or(f, u"opoint"))) {
    for (size_t i = 0; i < opoint->size(); ++i) {
      ret->insert(xml_x_opoint(xml, opoint->at(i), u"opoint"));
    }
  }
  ret->insert(xml_x_bpoint(xml, field_or(f, u"bpoint"), u"bpoint"));
  ret->insert(xml_x_wpoint(xml, field_or(f, u"wpoint"), u"wpoint"));
  ret->insert(xml_x_cpoint(xml, field_or(f, u"cpoint"), u"cpoint"));
  ret->insert(xml_x_chase(xml, field_or(f, u"chase"), u"chase"));

  set_label_attr(ret, u"FRAME_BEHAVIOR_LABEL_MAP", u"behavior_label", field_or(f, u"behavior"));
  set_label_attr(ret, u"StateEnumNames", u"state_label", field_or(f, u"state"));
  return ret;
}

Value xml_2_frame(const IXMLElement& el) {
  Value ret = frame_info_new();
  Object* const o = as_object(ret);
  o->set(u"id", from_opt(get_str_or(el, u"id", field_or(*o, u"id"))));
  o->set(u"name", from_opt(get_str_or(el, u"name", field_or(*o, u"name"))));
  o->set(u"ref", from_opt(get_str_or(el, u"ref", field_or(*o, u"ref"))));

  {
    const std::vector<IXMLElement*> pic_els = el.children_by_tag(u"pic");
    auto pics = std::make_shared<Array>();
    for (IXMLElement* v : pic_els) pics->push_back(xml_2_frame_pic(*v));
    if (pics->size() > 0) o->set(u"pic", pics->at(0));
    if (pics->size() > 1) {
      auto tail = std::make_shared<Array>();
      for (size_t i = 1; i < pics->size(); ++i) tail->push_back(pics->at(i));
      o->set(u"pics", Value(std::move(tail)));
    }
  }

  o->set(u"model", xml_2_frame_model(el.child_by_tag(u"model")));

  o->set(u"state", from_opt(get_num_or(el, u"state", field_or(*o, u"state"))));
  o->set(u"wait", from_opt(get_num_or(el, u"wait", field_or(*o, u"wait"))));
  {
    const std::optional<std::vector<double>> center = el.nums_attr(u"center");
    o->set(u"centerx", from_opt(get_num_or(el, u"centerx", arr_at(center, 0, field_or(*o, u"centerx")))));
    o->set(u"centery", from_opt(get_num_or(el, u"centery", arr_at(center, 1, field_or(*o, u"centery")))));
  }
  {
    const std::optional<std::vector<double>> size = el.nums_attr(u"size");
    o->set(u"width", from_opt(get_num_or(el, u"width", arr_at(size, 0, field_or(*o, u"width")))));
    o->set(u"height", from_opt(get_num_or(el, u"height", arr_at(size, 1, field_or(*o, u"height")))));
  }
  o->set(u"sound", one_or_arr(el.get_str_arr(u"sound")));
  o->set(u"hp", from_opt(get_num_or(el, u"hp", field_or(*o, u"hp"))));
  o->set(u"mp", from_opt(get_num_or(el, u"mp", field_or(*o, u"mp"))));
  o->set(u"invisible", from_opt(el.get_num(u"invisible")));
  o->set(u"invulnerable", from_opt(el.get_num(u"invulnerable")));
  o->set(u"blinking", from_opt(el.get_num(u"blinking")));
  o->set(u"bot_ignore", from_opt(el.get_num(u"bot_ignore")));
  o->set(u"no_shadow", from_opt(el.get_num(u"no_shadow")));
  o->set(u"jump_flag", from_opt(el.get_num(u"jump_flag")));
  o->set(u"behavior", from_opt(el.get_num(u"behavior")));
  o->set(u"landable", from_opt(el.get_num(u"landable")));
  o->set(u"facing", from_opt(el.get_num(u"facing")));
  o->set(u"gravity_enabled", from_opt(el.get_bool(u"gravity_enabled")));
  o->set(u"stat_recover", from_opt(el.get_num(u"stat_recover")));
  o->set(u"toughness_recover", from_opt(el.get_num(u"toughness_recover")));
  (void)xml_to_velocity_info(el, ret);

  o->set(u"next", xml_2_t_next_frame(el.children_by_tag(u"next")));
  o->set(u"on_dead", xml_2_t_next_frame(el.children_by_tag(u"on_dead")));
  o->set(u"on_landing", xml_2_t_next_frame(el.children_by_tag(u"on_landing")));
  o->set(u"on_exhaustion", xml_2_t_next_frame(el.children_by_tag(u"on_exhaustion")));
  o->set(u"bdy", xml_2_non_empty(el, u"bdy", xml_2_bdy));
  o->set(u"itr", xml_2_non_empty(el, u"itr", xml_2_itr));
  o->set(u"opoint", xml_2_non_empty(el, u"opoint", xml_2_opoint));
  o->set(u"wpoint", merge_by_tag(el, u"wpoint", xml_2_wpoint));
  o->set(u"bpoint", merge_by_tag(el, u"bpoint", xml_2_bpoint));
  o->set(u"cpoint", merge_by_tag(el, u"cpoint", xml_2_cpoint));
  o->set(u"chase", merge_by_tag(el, u"chase", xml_2_chase));
  o->set(u"hit", xml_2_hit_key_map(el, u"hit"));
  o->set(u"hold", xml_2_hit_key_map(el, u"hold"));
  o->set(u"key_down", xml_2_hit_key_map(el, u"key_down"));
  o->set(u"key_up", xml_2_hit_key_map(el, u"key_up"));
  o->set(u"seqs", xml_2_hit_key_map(el, u"seqs"));
  o->set(u"dataset", xml_to_world_dataset(el.child_by_tag(u"dataset")));

  return delete_undefined(ret);
}

}
}
}
