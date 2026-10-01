#pragma once

#include <vector>

#include "lfw/defines/armor_enum.h"
#include "lfw/defines/background_group.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/bot_state_enum.h"
#include "lfw/defines/bot_val.h"
#include "lfw/defines/c_point_kind.h"
#include "lfw/defines/chase_lost.h"
#include "lfw/defines/chase_strategy.h"
#include "lfw/defines/cheat_type.h"
#include "lfw/defines/collision_val.h"
#include "lfw/defines/ctrl_device.h"
#include "lfw/defines/difficulty.h"
#include "lfw/defines/entity_group.h"
#include "lfw/defines/entity_val.h"
#include "lfw/defines/frame_behavior.h"
#include "lfw/defines/frame_id.h"
#include "lfw/defines/game_key.h"
#include "lfw/defines/i_dat_index.h"
#include "lfw/defines/i_dialog_info.h"
#include "lfw/defines/itr_effect.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/lf2_val.h"
#include "lfw/defines/magnification_texture_filter.h"
#include "lfw/defines/minification_texture_filter.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/opoint_kind.h"
#include "lfw/defines/opoint_multi_enum.h"
#include "lfw/defines/opoint_spreading.h"
#include "lfw/defines/spark_enum.h"
#include "lfw/defines/speed_ctrl.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/defines/stage_actions.h"
#include "lfw/defines/stage_group.h"
#include "lfw/defines/stage_val.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/sync_render_enum.h"
#include "lfw/defines/team_enum.h"
#include "lfw/defines/texture_wrapping.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/defines/world_val.h"
#include "lfw/defines/wpoint_kind.h"
#include "lfw/defines/action_type.h"
#include "lfw/defines/defines.h"

#include "lfw/defines/enum_entries.h"

#include "lfw/defines/all_enums_extra.h"

namespace lfw {

inline const std::vector<EnumNumberTableRef>& all_number_enum_tables() {
  static const std::vector<EnumNumberTableRef> t = [] {
    std::vector<EnumNumberTableRef> v = {
      {u"ArmorEnum", &armor_enum_entries(), &armor_enum_name_of},
      {u"BdyKind", &bdy_kind_entries(), &bdy_kind_name_of},
      {u"CPointKind", &c_point_kind_entries(), &c_point_kind_name_of},
      {u"ChaseLost", &chase_lost_entries(), &chase_lost_name_of},
      {u"ChaseStrategy", &chase_strategy_entries(), &chase_strategy_name_of},
      {u"CtrlDevice", &ctrl_device_entries(), &ctrl_device_name_of},
      {u"Difficulty", &difficulty_entries(), &difficulty_name_of},
      {u"FrameBehavior", &frame_behavior_entries(), &frame_behavior_name_of},
      {u"ItrEffect", &itr_effect_entries(), &itr_effect_name_of},
      {u"ItrKind", &itr_kind_entries(), &itr_kind_name_of},
      {u"MagnificationTextureFilter", &magnification_texture_filter_entries(), &magnification_texture_filter_name_of},
      {u"MinificationTextureFilter", &minification_texture_filter_entries(), &minification_texture_filter_name_of},
      {u"OpointKind", &opoint_kind_entries(), &opoint_kind_name_of},
      {u"OpointMultiEnum", &opoint_multi_enum_entries(), &opoint_multi_enum_name_of},
      {u"OpointSpreading", &opoint_spreading_entries(), &opoint_spreading_name_of},
      {u"SpeedCtrl", &speed_ctrl_entries(), &speed_ctrl_name_of},
      {u"SpeedMode", &speed_mode_entries(), &speed_mode_name_of},
      {u"StateEnum", &state_enum_entries(), &state_enum_name_of},
      {u"SyncRenderEnum", &sync_render_enum_entries(), &sync_render_enum_name_of},
      {u"TextureWrapping", &texture_wrapping_entries(), &texture_wrapping_name_of},
      {u"WeaponEnum", &weapon_enum_entries(), &weapon_enum_name_of},
      {u"WpointKind", &wpoint_kind_entries(), &wpoint_kind_name_of},
    };
    append_extra_number_enum_tables(v);
    return v;
  }();
  return t;
}

inline const std::vector<EnumTextTableRef>& all_text_enum_tables() {
  static const std::vector<EnumTextTableRef> t = [] {
    std::vector<EnumTextTableRef> v = {
      {u"BackgroundGroup", &background_group_entries()},
      {u"BotStateEnum", &bot_state_enum_entries()},
      {u"BotVal", &bot_val_entries()},
      {u"CheatEnum", &cheat_enum_entries()},
      {u"CollisionVal", &collision_val_entries()},
      {u"EntityGroup", &entity_group_entries()},
      {u"EntityVal", &entity_val_entries()},
      {u"FrameId", &frame_id_entries()},
      {u"GK", &gk_entries()},
      {u"DatTypeEnum", &dat_type_enum_entries()},
      {u"DialogCloseBy", &dialog_close_by_entries()},
      {u"LF2Val", &lf2_val_entries()},
      {u"OID", &oid_entries()},
      {u"SparkEnum", &spark_enum_entries()},
      {u"StageActions", &stage_actions_entries()},
      {u"StageGroup", &stage_group_entries()},
      {u"StageVal", &stage_val_entries()},
      {u"TeamEnum", &team_enum_entries()},
      {u"WorldVal", &world_val_entries()},
      {u"ActionType", &action_type_entries()},
      {u"BuiltIn_Imgs", &built_in_imgs_entries()},
      {u"BuiltIn_Dats", &built_in_dats_entries()},
      {u"BuiltIn_Broadcast", &built_in_broadcast_entries()},
      {u"BuiltIn_Sounds", &built_in_sounds_entries()},
    };
    append_extra_text_enum_tables(v);
    return v;
  }();
  return t;
}

}
