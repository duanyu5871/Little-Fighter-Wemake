#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace action_type {

inline constexpr const char16_t* kA_SOUND = u"A_SOUND";
inline constexpr const char16_t* kA_NEXT_FRAME = u"A_NEXT_FRAME";
inline constexpr const char16_t* kA_SET_PROP = u"A_SET_PROP";
inline constexpr const char16_t* kA_DEFEND = u"A_DEFEND";
inline constexpr const char16_t* kA_BROKEN_DEFEND = u"A_BROKEN_DEFEND";
inline constexpr const char16_t* kV_SOUND = u"V_SOUND";
inline constexpr const char16_t* kV_NEXT_FRAME = u"V_NEXT_FRAME";
inline constexpr const char16_t* kV_SET_PROP = u"V_SET_PROP";
inline constexpr const char16_t* kV_DEFEND = u"V_DEFEND";
inline constexpr const char16_t* kV_BROKEN_DEFEND = u"V_BROKEN_DEFEND";
inline constexpr const char16_t* kA_REBOUND_VX = u"A_REBOUND_VX";
inline constexpr const char16_t* kV_REBOUND_VX = u"V_REBOUND_VX";
inline constexpr const char16_t* kV_TURN_FACE = u"V_TURN_FACE";
inline constexpr const char16_t* kV_TURN_TEAM = u"V_TURN_TEAM";
inline constexpr const char16_t* kFUSION = u"FUSION";
inline constexpr const char16_t* kBROADCAST = u"BROADCAST";
inline constexpr const char16_t* kVALUE_STEAL = u"VALUE_STEAL";
inline constexpr const char16_t* kA_BUFF = u"A_BUFF";
inline constexpr const char16_t* kV_BUFF = u"V_BUFF";
inline constexpr const char16_t* kERROR = u"ERROR";
inline constexpr const char16_t* kNONE = u"NONE";

}

inline const std::vector<EnumTextEntry>& action_type_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"A_SOUND", action_type::kA_SOUND},
    {u"A_NEXT_FRAME", action_type::kA_NEXT_FRAME},
    {u"A_SET_PROP", action_type::kA_SET_PROP},
    {u"A_DEFEND", action_type::kA_DEFEND},
    {u"A_BROKEN_DEFEND", action_type::kA_BROKEN_DEFEND},
    {u"V_SOUND", action_type::kV_SOUND},
    {u"V_NEXT_FRAME", action_type::kV_NEXT_FRAME},
    {u"V_SET_PROP", action_type::kV_SET_PROP},
    {u"V_DEFEND", action_type::kV_DEFEND},
    {u"V_BROKEN_DEFEND", action_type::kV_BROKEN_DEFEND},
    {u"A_REBOUND_VX", action_type::kA_REBOUND_VX},
    {u"V_REBOUND_VX", action_type::kV_REBOUND_VX},
    {u"V_TURN_FACE", action_type::kV_TURN_FACE},
    {u"V_TURN_TEAM", action_type::kV_TURN_TEAM},
    {u"FUSION", action_type::kFUSION},
    {u"BROADCAST", action_type::kBROADCAST},
    {u"VALUE_STEAL", action_type::kVALUE_STEAL},
    {u"A_BUFF", action_type::kA_BUFF},
    {u"V_BUFF", action_type::kV_BUFF},
    {u"ERROR", action_type::kERROR},
    {u"NONE", action_type::kNONE},
  };
  return e;
}

}
