#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class SpeedMode : int {
  Default = 0,
  Acc = 1,
  FixedLf2 = 2,
  FixedAcc = 3,
  AccTo = 4,
  Extra = 5,
  Fixed = 6,
  FixedAccTo = 7,
};

inline const std::vector<EnumNumberEntry>& speed_mode_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Default", static_cast<double>(SpeedMode::Default)},
    {u"Acc", static_cast<double>(SpeedMode::Acc)},
    {u"FixedLf2", static_cast<double>(SpeedMode::FixedLf2)},
    {u"FixedAcc", static_cast<double>(SpeedMode::FixedAcc)},
    {u"AccTo", static_cast<double>(SpeedMode::AccTo)},
    {u"Extra", static_cast<double>(SpeedMode::Extra)},
    {u"Fixed", static_cast<double>(SpeedMode::Fixed)},
    {u"FixedAccTo", static_cast<double>(SpeedMode::FixedAccTo)},
  };
  return e;
}

inline const char16_t* speed_mode_name_of(int v) {
  switch (v) {
    case static_cast<int>(SpeedMode::Default): return u"Default";
    case static_cast<int>(SpeedMode::Acc): return u"Acc";
    case static_cast<int>(SpeedMode::FixedLf2): return u"FixedLf2";
    case static_cast<int>(SpeedMode::FixedAcc): return u"FixedAcc";
    case static_cast<int>(SpeedMode::AccTo): return u"AccTo";
    case static_cast<int>(SpeedMode::Extra): return u"Extra";
    case static_cast<int>(SpeedMode::Fixed): return u"Fixed";
    case static_cast<int>(SpeedMode::FixedAccTo): return u"FixedAccTo";
  }
  return nullptr;
}

}
