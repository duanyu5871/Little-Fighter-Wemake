#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class SpeedCtrl : int {
  None = 0,
  Control = 1,
  Enable = 2,
  Disable = 3,
};

inline const std::vector<EnumNumberEntry>& speed_ctrl_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"None", static_cast<double>(SpeedCtrl::None)},
    {u"Control", static_cast<double>(SpeedCtrl::Control)},
    {u"Enable", static_cast<double>(SpeedCtrl::Enable)},
    {u"Disable", static_cast<double>(SpeedCtrl::Disable)},
  };
  return e;
}

inline const char16_t* speed_ctrl_name_of(int v) {
  switch (v) {
    case static_cast<int>(SpeedCtrl::None): return u"None";
    case static_cast<int>(SpeedCtrl::Control): return u"Control";
    case static_cast<int>(SpeedCtrl::Enable): return u"Enable";
    case static_cast<int>(SpeedCtrl::Disable): return u"Disable";
  }
  return nullptr;
}

}
