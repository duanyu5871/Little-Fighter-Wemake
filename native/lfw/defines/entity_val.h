#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace entity_val {

inline constexpr const char16_t* kTrendX = u"trend_x";
inline constexpr const char16_t* kPressFB = u"press_F_B";
inline constexpr const char16_t* kPressUD = u"press_U_D";
inline constexpr const char16_t* kPressLR = u"press_L_R";
inline constexpr const char16_t* kHolding_W_Type = u"holding_w_type";
inline constexpr const char16_t* kHP_P = u"hp_p";
inline constexpr const char16_t* kLF2_NET_ON = u"lf2_net_on";
inline constexpr const char16_t* kHERO_FT_ON = u"hero_ft_on";
inline constexpr const char16_t* kGIM_INK_ON = u"gim_ink_on";
inline constexpr const char16_t* kHAS_TRANSFORM_DATA = u"has_transform_data";
inline constexpr const char16_t* kCatching = u"catching";
inline constexpr const char16_t* kCAUGHT = u"caught";
inline constexpr const char16_t* kRequireSuperPunch = u"super_punch";
inline constexpr const char16_t* kHitByCharacter = u"hit_by_character";
inline constexpr const char16_t* kHitByWeapon = u"hit_by_weapon";
inline constexpr const char16_t* kHitByBall = u"hit_by_ball";
inline constexpr const char16_t* kHitByState = u"hit_by_state";
inline constexpr const char16_t* kHitByItrKind = u"hit_by_itr_kind";
inline constexpr const char16_t* kHitByItrEffect = u"hit_by_itr_effect";
inline constexpr const char16_t* kHitOnCharacter = u"hit_on_character";
inline constexpr const char16_t* kHitOnWeapon = u"hit_on_weapon";
inline constexpr const char16_t* kHitOnBall = u"hit_on_ball";
inline constexpr const char16_t* kHitOnState = u"hit_on_state";
inline constexpr const char16_t* kHitOnSth = u"hit_on_something";
inline constexpr const char16_t* kHP = u"hp";
inline constexpr const char16_t* kMP = u"mp";
inline constexpr const char16_t* kVX = u"vx";
inline constexpr const char16_t* kVY = u"vy";
inline constexpr const char16_t* kVZ = u"vz";
inline constexpr const char16_t* kFrameState = u"frame_state";
inline constexpr const char16_t* kShaking = u"shaking";
inline constexpr const char16_t* kHolding = u"holding";
inline constexpr const char16_t* kHoldingHeavy = u"holdingHeavy";
inline constexpr const char16_t* kHoldingOID = u"holdingOID";
inline constexpr const char16_t* kHpRecoverable = u"hp_recoverable";
inline constexpr const char16_t* kHitByMagicFlute = u"hit_by_magic_flute";
inline constexpr const char16_t* kTransformListSize = u"transform_list_size";
inline constexpr const char16_t* kIsOnGround = u"is_on_ground";
inline constexpr const char16_t* kTransformIndex = u"transform_index";
inline constexpr const char16_t* kIsSurvialRankMode = u"survial_rank_mode";

}

inline const std::vector<EnumTextEntry>& entity_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"TrendX", entity_val::kTrendX},
    {u"PressFB", entity_val::kPressFB},
    {u"PressUD", entity_val::kPressUD},
    {u"PressLR", entity_val::kPressLR},
    {u"Holding_W_Type", entity_val::kHolding_W_Type},
    {u"HP_P", entity_val::kHP_P},
    {u"LF2_NET_ON", entity_val::kLF2_NET_ON},
    {u"HERO_FT_ON", entity_val::kHERO_FT_ON},
    {u"GIM_INK_ON", entity_val::kGIM_INK_ON},
    {u"HAS_TRANSFORM_DATA", entity_val::kHAS_TRANSFORM_DATA},
    {u"Catching", entity_val::kCatching},
    {u"CAUGHT", entity_val::kCAUGHT},
    {u"RequireSuperPunch", entity_val::kRequireSuperPunch},
    {u"HitByCharacter", entity_val::kHitByCharacter},
    {u"HitByWeapon", entity_val::kHitByWeapon},
    {u"HitByBall", entity_val::kHitByBall},
    {u"HitByState", entity_val::kHitByState},
    {u"HitByItrKind", entity_val::kHitByItrKind},
    {u"HitByItrEffect", entity_val::kHitByItrEffect},
    {u"HitOnCharacter", entity_val::kHitOnCharacter},
    {u"HitOnWeapon", entity_val::kHitOnWeapon},
    {u"HitOnBall", entity_val::kHitOnBall},
    {u"HitOnState", entity_val::kHitOnState},
    {u"HitOnSth", entity_val::kHitOnSth},
    {u"HP", entity_val::kHP},
    {u"MP", entity_val::kMP},
    {u"VX", entity_val::kVX},
    {u"VY", entity_val::kVY},
    {u"VZ", entity_val::kVZ},
    {u"FrameState", entity_val::kFrameState},
    {u"Shaking", entity_val::kShaking},
    {u"Holding", entity_val::kHolding},
    {u"HoldingHeavy", entity_val::kHoldingHeavy},
    {u"HoldingOID", entity_val::kHoldingOID},
    {u"HpRecoverable", entity_val::kHpRecoverable},
    {u"HitByMagicFlute", entity_val::kHitByMagicFlute},
    {u"TransformListSize", entity_val::kTransformListSize},
    {u"IsOnGround", entity_val::kIsOnGround},
    {u"TransformIndex", entity_val::kTransformIndex},
    {u"IsSurvialRankMode", entity_val::kIsSurvialRankMode},
  };
  return e;
}

}
