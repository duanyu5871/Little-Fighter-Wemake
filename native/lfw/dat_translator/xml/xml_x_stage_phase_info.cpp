#include "lfw/dat_translator/xml/xml_x_stage_phase_info.h"

#include "lfw/core/js_string.h"
#include "lfw/dat_translator/xml/delete_undefined.h"
#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/dat_translator/xml/xml_x_dialog_info.h"
#include "lfw/dat_translator/xml/xml_x_difficulty_map.h"
#include "lfw/dat_translator/xml/xml_x_non_empty.h"
#include "lfw/dat_translator/xml/xml_x_stage_object_info.h"
#include "lfw/defines/fields_gen.h"
#include "lfw/defines/runtime_gen.h"
#include "lfw/fields.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_x_sound_play_info(IXML& xml, const Value& s,
                                                   const std::u16string& s_tag) {
  // ⚠ TS 原文 `if (!s || s.path.trim()) return;` —— path 有内容（trim 后非空）就返回
  // undefined，只有 path 为空串/空白时才写得出来。照抄。
  const std::optional<std::u16string> path = opt_str(field_or(s, u"path"));
  bool path_blank = true;
  if (path) {
    for (char16_t c : *path) {
      if (!is_str_white_space(c)) {
        path_blank = false;
        break;
      }
    }
  }
  if (!truthy(s) || !path_blank) return nullptr;
  std::shared_ptr<IXMLElement> el = xml.create(s_tag);
  el->set_attr(u"path", field_or(s, u"path"));
  el->set_attr(u"x", field_or(s, u"x"));
  el->set_attr(u"y", field_or(s, u"y"));
  el->set_attr(u"z", field_or(s, u"z"));
  el->set_attr(u"desc", field_or(s, u"desc"));
  return el;
}

Value xml_2_sound_play_info(const IXMLElement& el) {
  Value ret = sound_play_info_new();
  Object* const o = as_object(ret);
  o->set(u"path", from_opt(get_str_or(el, u"path", field_or(*o, u"path"))));
  o->set(u"x", from_opt(get_num_or(el, u"x", field_or(*o, u"x"))));
  o->set(u"y", from_opt(get_num_or(el, u"y", field_or(*o, u"y"))));
  o->set(u"z", from_opt(get_num_or(el, u"z", field_or(*o, u"z"))));
  o->set(u"desc", from_opt(get_str_or(el, u"desc", field_or(*o, u"desc"))));
  return ret;
}

std::shared_ptr<IXMLElement> xml_x_stage_phase_info(IXML& xml, const Value& p,
                                                    const std::u16string& tag) {
  std::shared_ptr<IXMLElement> el = xml.create(tag);
  el->set_attr(u"title", field_or(p, u"title"));
  el->set_attr(u"desc", field_or(p, u"desc"));
  el->set_attr(u"bound", field_or(p, u"bound"));
  el->set_attr(u"player_l", field_or(p, u"player_l"));
  el->set_attr(u"player_r", field_or(p, u"player_r"));
  el->set_attr(u"camera_l", field_or(p, u"camera_l"));
  el->set_attr(u"camera_r", field_or(p, u"camera_r"));
  el->set_attr(u"enemy_l", field_or(p, u"enemy_l"));
  el->set_attr(u"enemy_r", field_or(p, u"enemy_r"));
  el->set_attr(u"drink_l", field_or(p, u"drink_l"));
  el->set_attr(u"drink_r", field_or(p, u"drink_r"));
  el->set_attr(u"music", field_or(p, u"music"));
  el->set_attr(u"cam_jump_to_x", field_or(p, u"cam_jump_to_x"));
  el->set_attr(u"player_jump_to_x", field_or(p, u"player_jump_to_x"));
  el->set_attr(u"player_jump_to_z", field_or(p, u"player_jump_to_z"));
  el->set_attr(u"player_facing", field_or(p, u"player_facing"));
  el->set_attr(u"end_test", field_or(p, u"end_test"));
  el->set_attr(u"on_start", field_or(p, u"on_start"));
  el->set_attr(u"on_end", field_or(p, u"on_end"));
  el->set_attr(u"hide_stats", field_or(p, u"hide_stats"));
  el->set_attr(u"world_pause", field_or(p, u"world_pause"));
  el->set_attr(u"control_disabled", field_or(p, u"control_disabled"));
  el->set_attr(u"weapon_rain_disabled", field_or(p, u"weapon_rain_disabled"));
  xml_x_difficulty_map(el, u"respawn", field_or(p, u"respawn"));
  xml_x_difficulty_map(el, u"respawn_r", field_or(p, u"respawn_r"));
  xml_x_difficulty_map(el, u"respawn_x", field_or(p, u"respawn_x"));
  xml_x_difficulty_map(el, u"health_up", field_or(p, u"health_up"));
  xml_x_difficulty_map(el, u"mp_up", field_or(p, u"mp_up"));
  xml_x_non_empty(xml, field_or(p, u"sounds"), u"sound", xml_x_sound_play_info, el);
  xml_x_non_empty(xml, field_or(p, u"objects"), u"object", xml_x_stage_object_info, el);
  xml_x_non_empty(xml, field_or(p, u"dialogs"), u"dialog", xml_x_dialog_info, el);
  return el;
}

Value xml_2_stage_phase_info(const IXMLElement& el) {
  Value ret = stage_phase_info_new();
  Object* const o = as_object(ret);
  o->set(u"title", from_opt(get_str_or(el, u"title", field_or(*o, u"title"))));
  o->set(u"desc", from_opt(get_str_or(el, u"desc", field_or(*o, u"desc"))));
  o->set(u"music", from_opt(get_str_or(el, u"music", field_or(*o, u"music"))));
  o->set(u"bound", from_opt(get_num_or(el, u"bound", field_or(*o, u"bound"))));
  o->set(u"player_l", from_opt(get_num_or(el, u"player_l", field_or(*o, u"player_l"))));
  o->set(u"player_r", from_opt(get_num_or(el, u"player_r", field_or(*o, u"player_r"))));
  o->set(u"camera_l", from_opt(get_num_or(el, u"camera_l", field_or(*o, u"camera_l"))));
  o->set(u"camera_r", from_opt(get_num_or(el, u"camera_r", field_or(*o, u"camera_r"))));
  o->set(u"enemy_l", from_opt(get_num_or(el, u"enemy_l", field_or(*o, u"enemy_l"))));
  o->set(u"enemy_r", from_opt(get_num_or(el, u"enemy_r", field_or(*o, u"enemy_r"))));
  o->set(u"drink_l", from_opt(get_num_or(el, u"drink_l", field_or(*o, u"drink_l"))));
  o->set(u"drink_r", from_opt(get_num_or(el, u"drink_r", field_or(*o, u"drink_r"))));
  o->set(u"cam_jump_to_x", from_opt(get_num_or(el, u"cam_jump_to_x", field_or(*o, u"cam_jump_to_x"))));
  o->set(u"player_jump_to_x",
         from_opt(get_num_or(el, u"player_jump_to_x", field_or(*o, u"player_jump_to_x"))));
  o->set(u"player_jump_to_z",
         from_opt(get_num_or(el, u"player_jump_to_z", field_or(*o, u"player_jump_to_z"))));
  o->set(u"player_facing", from_opt(get_num_or(el, u"player_facing", field_or(*o, u"player_facing"))));
  o->set(u"end_test", from_opt(el.get_str_arr(u"end_test")));
  o->set(u"on_start", from_opt(el.get_str_arr(u"on_start")));
  o->set(u"on_end", from_opt(el.get_str_arr(u"on_end")));
  o->set(u"hide_stats", from_opt(get_num_or(el, u"hide_stats", field_or(*o, u"hide_stats"))));
  o->set(u"world_pause", from_opt(get_num_or(el, u"world_pause", field_or(*o, u"world_pause"))));
  o->set(u"control_disabled",
         from_opt(get_num_or(el, u"control_disabled", field_or(*o, u"control_disabled"))));
  o->set(u"weapon_rain_disabled",
         from_opt(get_num_or(el, u"weapon_rain_disabled", field_or(*o, u"weapon_rain_disabled"))));

  o->set(u"respawn", xml_2_difficulty_map(el, u"respawn"));
  o->set(u"respawn_r", xml_2_difficulty_map(el, u"respawn_r"));
  o->set(u"respawn_x", xml_2_difficulty_map(el, u"respawn_x"));
  o->set(u"health_up", xml_2_difficulty_map(el, u"health_up"));
  o->set(u"mp_up", xml_2_difficulty_map(el, u"mp_up"));

  // ⚠ TS 是三条**裸调用**：结果被丢掉，sounds/objects/dialogs 不进结果对象。照抄。
  (void)xml_2_non_empty(el, u"sound", xml_2_sound_play_info);
  (void)xml_2_non_empty(el, u"object", xml_2_stage_object_info);
  (void)xml_2_non_empty(el, u"dialog", xml_2_dialog_info);

  delete_undefined(ret);
  reorder_fields(ret, stage_phase_info_fields());
  return ret;
}

}
}
}
