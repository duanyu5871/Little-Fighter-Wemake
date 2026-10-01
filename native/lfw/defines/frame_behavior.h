#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class FrameBehavior : int {
  JohnChase = 1,
  DennisChase = 2,
  Boomerang = 3,
  AngelBlessing = 4,
  AngelBlessingStart = 5,
  DevilJudgementStart = 6,
  ChasingSameEnemy = 7,
  BatStart = 8,
  FirzenDisasterStart = 9,
  JohnBiscuitLeaving = 10,
  FirzenVolcanoStart = 11,
  Bat = 12,
  JulianBallStart = 13,
  JulianBall = 14,
};

inline const std::vector<EnumNumberEntry>& frame_behavior_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"JohnChase", static_cast<double>(FrameBehavior::JohnChase)},
    {u"DennisChase", static_cast<double>(FrameBehavior::DennisChase)},
    {u"Boomerang", static_cast<double>(FrameBehavior::Boomerang)},
    {u"AngelBlessing", static_cast<double>(FrameBehavior::AngelBlessing)},
    {u"AngelBlessingStart", static_cast<double>(FrameBehavior::AngelBlessingStart)},
    {u"DevilJudgementStart", static_cast<double>(FrameBehavior::DevilJudgementStart)},
    {u"ChasingSameEnemy", static_cast<double>(FrameBehavior::ChasingSameEnemy)},
    {u"BatStart", static_cast<double>(FrameBehavior::BatStart)},
    {u"FirzenDisasterStart", static_cast<double>(FrameBehavior::FirzenDisasterStart)},
    {u"JohnBiscuitLeaving", static_cast<double>(FrameBehavior::JohnBiscuitLeaving)},
    {u"FirzenVolcanoStart", static_cast<double>(FrameBehavior::FirzenVolcanoStart)},
    {u"Bat", static_cast<double>(FrameBehavior::Bat)},
    {u"JulianBallStart", static_cast<double>(FrameBehavior::JulianBallStart)},
    {u"JulianBall", static_cast<double>(FrameBehavior::JulianBall)},
  };
  return e;
}

inline const char16_t* frame_behavior_name_of(int v) {
  switch (v) {
    case static_cast<int>(FrameBehavior::JohnChase): return u"JohnChase";
    case static_cast<int>(FrameBehavior::DennisChase): return u"DennisChase";
    case static_cast<int>(FrameBehavior::Boomerang): return u"Boomerang";
    case static_cast<int>(FrameBehavior::AngelBlessing): return u"AngelBlessing";
    case static_cast<int>(FrameBehavior::AngelBlessingStart): return u"AngelBlessingStart";
    case static_cast<int>(FrameBehavior::DevilJudgementStart): return u"DevilJudgementStart";
    case static_cast<int>(FrameBehavior::ChasingSameEnemy): return u"ChasingSameEnemy";
    case static_cast<int>(FrameBehavior::BatStart): return u"BatStart";
    case static_cast<int>(FrameBehavior::FirzenDisasterStart): return u"FirzenDisasterStart";
    case static_cast<int>(FrameBehavior::JohnBiscuitLeaving): return u"JohnBiscuitLeaving";
    case static_cast<int>(FrameBehavior::FirzenVolcanoStart): return u"FirzenVolcanoStart";
    case static_cast<int>(FrameBehavior::Bat): return u"Bat";
    case static_cast<int>(FrameBehavior::JulianBallStart): return u"JulianBallStart";
    case static_cast<int>(FrameBehavior::JulianBall): return u"JulianBall";
  }
  return nullptr;
}

}
