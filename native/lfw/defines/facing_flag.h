#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class FacingFlag : int {
  None = 0,
  L = -1,
  Left = L,
  R = 1,
  Right = R,
  B = 2,
  Backward = B,
  Ctrl = 3,
  SameAsCatcher = 4,
  OpposingCatcher = 5,
  AntiCtrl = 6,
  VX = 7,
  AntiVX = 8,
  Trend = 9,
  SameAsBearer = 4,
  OpposingBearer = 5,
};

inline const std::vector<EnumNumberEntry>& facing_flag_entries() {
  static const std::vector<EnumNumberEntry> e = {
      {u"None", static_cast<double>(FacingFlag::None)},
      {u"L", static_cast<double>(FacingFlag::L)},
      {u"Left", static_cast<double>(FacingFlag::Left)},
      {u"R", static_cast<double>(FacingFlag::R)},
      {u"Right", static_cast<double>(FacingFlag::Right)},
      {u"B", static_cast<double>(FacingFlag::B)},
      {u"Backward", static_cast<double>(FacingFlag::Backward)},
      {u"Ctrl", static_cast<double>(FacingFlag::Ctrl)},
      {u"SameAsCatcher", static_cast<double>(FacingFlag::SameAsCatcher)},
      {u"OpposingCatcher", static_cast<double>(FacingFlag::OpposingCatcher)},
      {u"AntiCtrl", static_cast<double>(FacingFlag::AntiCtrl)},
      {u"VX", static_cast<double>(FacingFlag::VX)},
      {u"AntiVX", static_cast<double>(FacingFlag::AntiVX)},
      {u"Trend", static_cast<double>(FacingFlag::Trend)},
      {u"SameAsBearer", static_cast<double>(FacingFlag::SameAsBearer)},
      {u"OpposingBearer", static_cast<double>(FacingFlag::OpposingBearer)},
  };
  return e;
}

inline const char16_t* facing_flag_name_of(int v) {
  switch (v) {
    case static_cast<int>(FacingFlag::Left):
      return u"Left";
    case static_cast<int>(FacingFlag::None):
      return u"None";
    case static_cast<int>(FacingFlag::Right):
      return u"Right";
    case static_cast<int>(FacingFlag::Backward):
      return u"Backward";
    case static_cast<int>(FacingFlag::Ctrl):
      return u"Ctrl";
    case static_cast<int>(FacingFlag::SameAsBearer):
      return u"SameAsBearer";
    case static_cast<int>(FacingFlag::OpposingBearer):
      return u"OpposingBearer";
    case static_cast<int>(FacingFlag::AntiCtrl):
      return u"AntiCtrl";
    case static_cast<int>(FacingFlag::VX):
      return u"VX";
    case static_cast<int>(FacingFlag::AntiVX):
      return u"AntiVX";
    case static_cast<int>(FacingFlag::Trend):
      return u"Trend";
  }
  return nullptr;
}

}
