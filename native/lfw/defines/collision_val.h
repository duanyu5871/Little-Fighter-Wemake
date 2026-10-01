#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace collision_val {

inline constexpr const char16_t* kAttackerType = u"attacker_type";
inline constexpr const char16_t* kVictimType = u"victim_type";
inline constexpr const char16_t* kVictimIsChasing = u"victim_is_chasing";
inline constexpr const char16_t* kItrEffect = u"itr_effect";
inline constexpr const char16_t* kItrKind = u"itr_kind";
inline constexpr const char16_t* kSameFacing = u"same_facing";
inline constexpr const char16_t* kAttackerState = u"attacker_state";
inline constexpr const char16_t* kVictimState = u"victim_state";
inline constexpr const char16_t* kAttackerHasHolder = u"attacker_has_holder";
inline constexpr const char16_t* kVictimHasHolder = u"victim_has_holder";
inline constexpr const char16_t* kAttackerHasHolding = u"attacker_has_holding";
inline constexpr const char16_t* kVictimHasHolding = u"victim_has_holding";
inline constexpr const char16_t* kSameTeam = u"same_team";
inline constexpr const char16_t* kAttackerOID = u"attacker_oid";
inline constexpr const char16_t* kVictimOID = u"victim_oid";
inline constexpr const char16_t* kBdyKind = u"bdy_kind";
inline constexpr const char16_t* kVictimFrameId = u"victim_frame_id";
inline constexpr const char16_t* kVictimFrameIndex_ICE = u"victim_frame_index_ice";
inline constexpr const char16_t* kItrFall = u"itr_fall";
inline constexpr const char16_t* kAttackerThrew = u"attacker_threw";
inline constexpr const char16_t* kVictimThrew = u"victim_threw";
inline constexpr const char16_t* kVictimIsFreezableBall = u"victim_freezable_ball";
inline constexpr const char16_t* kAttackerIsFreezableBall = u"attacker_freezable_ball";
inline constexpr const char16_t* kArmorWork = u"armor_work";
inline constexpr const char16_t* kV_FrameBehavior = u"v_frame_behavior";
inline constexpr const char16_t* kNoItrEffect = u"no_itr_effect";
inline constexpr const char16_t* kA_HP_P = u"a_hp_p";
inline constexpr const char16_t* kV_HP_P = u"v_hp_p";
inline constexpr const char16_t* kLF2_NET_ON = u"lf2_net_on";
inline constexpr const char16_t* kBdyHitFlag = u"bdy_hit_flag";
inline constexpr const char16_t* kItrHitFlag = u"itr_hit_flag";
inline constexpr const char16_t* kBdyCode = u"bdy_code";
inline constexpr const char16_t* kItrCode = u"itr_code";
inline constexpr const char16_t* kVToughness = u"v_toughness";
inline constexpr const char16_t* kAToughness = u"a_toughness";
inline constexpr const char16_t* kAttackerBaseType = u"attacker_base_type";
inline constexpr const char16_t* kVictimBaseType = u"victim_base_type";
inline constexpr const char16_t* kAClosingSpeedX = u"a_closing_speed_x";
inline constexpr const char16_t* kAClosingSpeedY = u"a_closing_speed_y";
inline constexpr const char16_t* kAClosingSpeedZ = u"a_closing_speed_z";
inline constexpr const char16_t* kAEmitter = u"a_emitter";
inline constexpr const char16_t* kVEmitter = u"v_emitter";
inline constexpr const char16_t* kAHitAttack = u"a_hit_attack";
inline constexpr const char16_t* kAHitJump = u"a_hit_jump";
inline constexpr const char16_t* kAHitDefend = u"a_hit_defend";
inline constexpr const char16_t* kAHitUp = u"a_hit_up";
inline constexpr const char16_t* kAHitDown = u"a_hit_down";
inline constexpr const char16_t* kAHitLeft = u"a_hit_left";
inline constexpr const char16_t* kAHitRight = u"a_hit_right";
inline constexpr const char16_t* kVHitAttack = u"v_hit_attack";
inline constexpr const char16_t* kVHitJump = u"v_hit_jump";
inline constexpr const char16_t* kVHitDefend = u"v_hit_defend";
inline constexpr const char16_t* kVHitUp = u"v_hit_up";
inline constexpr const char16_t* kVHitDown = u"v_hit_down";
inline constexpr const char16_t* kVHitLeft = u"v_hit_left";
inline constexpr const char16_t* kVHitRight = u"v_hit_right";
inline constexpr const char16_t* kAClickAttack = u"a_click_attack";
inline constexpr const char16_t* kAClickJump = u"a_click_jump";
inline constexpr const char16_t* kAClickDefend = u"a_click_defend";
inline constexpr const char16_t* kAClickUp = u"a_click_up";
inline constexpr const char16_t* kAClickDown = u"a_click_down";
inline constexpr const char16_t* kAClickLeft = u"a_click_left";
inline constexpr const char16_t* kAClickRight = u"a_click_right";
inline constexpr const char16_t* kVClickAttack = u"v_click_attack";
inline constexpr const char16_t* kVClickJump = u"v_click_jump";
inline constexpr const char16_t* kVClickDefend = u"v_click_defend";
inline constexpr const char16_t* kVClickUp = u"v_click_up";
inline constexpr const char16_t* kVClickDown = u"v_click_down";
inline constexpr const char16_t* kVClickLeft = u"v_click_left";
inline constexpr const char16_t* kVClickRight = u"v_click_right";
inline constexpr const char16_t* kADbcAttack = u"a_dbclick_attack";
inline constexpr const char16_t* kADbcJump = u"a_dbclick_jump";
inline constexpr const char16_t* kADbcDefend = u"a_dbclick_defend";
inline constexpr const char16_t* kADbcUp = u"a_dbclick_up";
inline constexpr const char16_t* kADbcDown = u"a_dbclick_down";
inline constexpr const char16_t* kADbcLeft = u"a_dbclick_left";
inline constexpr const char16_t* kADbcRight = u"a_dbclick_right";
inline constexpr const char16_t* kVDbcAttack = u"v_dbclick_attack";
inline constexpr const char16_t* kVDbcJump = u"v_dbclick_jump";
inline constexpr const char16_t* kVDbcDefend = u"v_dbclick_defend";
inline constexpr const char16_t* kVDbcUp = u"v_dbclick_up";
inline constexpr const char16_t* kVDbcDown = u"v_dbclick_down";
inline constexpr const char16_t* kVDbcLeft = u"v_dbclick_left";
inline constexpr const char16_t* kVDbcRight = u"v_dbclick_right";
inline constexpr const char16_t* kAFALLING = u"a_falling";
inline constexpr const char16_t* kVFALLING = u"v_falling";

}

inline const std::vector<EnumTextEntry>& collision_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"AttackerType", collision_val::kAttackerType},
    {u"VictimType", collision_val::kVictimType},
    {u"VictimIsChasing", collision_val::kVictimIsChasing},
    {u"ItrEffect", collision_val::kItrEffect},
    {u"ItrKind", collision_val::kItrKind},
    {u"SameFacing", collision_val::kSameFacing},
    {u"AttackerState", collision_val::kAttackerState},
    {u"VictimState", collision_val::kVictimState},
    {u"AttackerHasHolder", collision_val::kAttackerHasHolder},
    {u"VictimHasHolder", collision_val::kVictimHasHolder},
    {u"AttackerHasHolding", collision_val::kAttackerHasHolding},
    {u"VictimHasHolding", collision_val::kVictimHasHolding},
    {u"SameTeam", collision_val::kSameTeam},
    {u"AttackerOID", collision_val::kAttackerOID},
    {u"VictimOID", collision_val::kVictimOID},
    {u"BdyKind", collision_val::kBdyKind},
    {u"VictimFrameId", collision_val::kVictimFrameId},
    {u"VictimFrameIndex_ICE", collision_val::kVictimFrameIndex_ICE},
    {u"ItrFall", collision_val::kItrFall},
    {u"AttackerThrew", collision_val::kAttackerThrew},
    {u"VictimThrew", collision_val::kVictimThrew},
    {u"VictimIsFreezableBall", collision_val::kVictimIsFreezableBall},
    {u"AttackerIsFreezableBall", collision_val::kAttackerIsFreezableBall},
    {u"ArmorWork", collision_val::kArmorWork},
    {u"V_FrameBehavior", collision_val::kV_FrameBehavior},
    {u"NoItrEffect", collision_val::kNoItrEffect},
    {u"A_HP_P", collision_val::kA_HP_P},
    {u"V_HP_P", collision_val::kV_HP_P},
    {u"LF2_NET_ON", collision_val::kLF2_NET_ON},
    {u"BdyHitFlag", collision_val::kBdyHitFlag},
    {u"ItrHitFlag", collision_val::kItrHitFlag},
    {u"BdyCode", collision_val::kBdyCode},
    {u"ItrCode", collision_val::kItrCode},
    {u"VToughness", collision_val::kVToughness},
    {u"AToughness", collision_val::kAToughness},
    {u"AttackerBaseType", collision_val::kAttackerBaseType},
    {u"VictimBaseType", collision_val::kVictimBaseType},
    {u"AClosingSpeedX", collision_val::kAClosingSpeedX},
    {u"AClosingSpeedY", collision_val::kAClosingSpeedY},
    {u"AClosingSpeedZ", collision_val::kAClosingSpeedZ},
    {u"AEmitter", collision_val::kAEmitter},
    {u"VEmitter", collision_val::kVEmitter},
    {u"AHitAttack", collision_val::kAHitAttack},
    {u"AHitJump", collision_val::kAHitJump},
    {u"AHitDefend", collision_val::kAHitDefend},
    {u"AHitUp", collision_val::kAHitUp},
    {u"AHitDown", collision_val::kAHitDown},
    {u"AHitLeft", collision_val::kAHitLeft},
    {u"AHitRight", collision_val::kAHitRight},
    {u"VHitAttack", collision_val::kVHitAttack},
    {u"VHitJump", collision_val::kVHitJump},
    {u"VHitDefend", collision_val::kVHitDefend},
    {u"VHitUp", collision_val::kVHitUp},
    {u"VHitDown", collision_val::kVHitDown},
    {u"VHitLeft", collision_val::kVHitLeft},
    {u"VHitRight", collision_val::kVHitRight},
    {u"AClickAttack", collision_val::kAClickAttack},
    {u"AClickJump", collision_val::kAClickJump},
    {u"AClickDefend", collision_val::kAClickDefend},
    {u"AClickUp", collision_val::kAClickUp},
    {u"AClickDown", collision_val::kAClickDown},
    {u"AClickLeft", collision_val::kAClickLeft},
    {u"AClickRight", collision_val::kAClickRight},
    {u"VClickAttack", collision_val::kVClickAttack},
    {u"VClickJump", collision_val::kVClickJump},
    {u"VClickDefend", collision_val::kVClickDefend},
    {u"VClickUp", collision_val::kVClickUp},
    {u"VClickDown", collision_val::kVClickDown},
    {u"VClickLeft", collision_val::kVClickLeft},
    {u"VClickRight", collision_val::kVClickRight},
    {u"ADbcAttack", collision_val::kADbcAttack},
    {u"ADbcJump", collision_val::kADbcJump},
    {u"ADbcDefend", collision_val::kADbcDefend},
    {u"ADbcUp", collision_val::kADbcUp},
    {u"ADbcDown", collision_val::kADbcDown},
    {u"ADbcLeft", collision_val::kADbcLeft},
    {u"ADbcRight", collision_val::kADbcRight},
    {u"VDbcAttack", collision_val::kVDbcAttack},
    {u"VDbcJump", collision_val::kVDbcJump},
    {u"VDbcDefend", collision_val::kVDbcDefend},
    {u"VDbcUp", collision_val::kVDbcUp},
    {u"VDbcDown", collision_val::kVDbcDown},
    {u"VDbcLeft", collision_val::kVDbcLeft},
    {u"VDbcRight", collision_val::kVDbcRight},
    {u"AFALLING", collision_val::kAFALLING},
    {u"VFALLING", collision_val::kVFALLING},
  };
  return e;
}

}
