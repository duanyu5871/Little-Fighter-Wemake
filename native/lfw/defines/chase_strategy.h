#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class ChaseStrategy : int {
  Default = 0,
  UntilLost = 1,
  StopOnLost = 2,
};

inline const std::vector<EnumNumberEntry>& chase_strategy_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Default", static_cast<double>(ChaseStrategy::Default)},
    {u"UntilLost", static_cast<double>(ChaseStrategy::UntilLost)},
    {u"StopOnLost", static_cast<double>(ChaseStrategy::StopOnLost)},
  };
  return e;
}

inline const char16_t* chase_strategy_name_of(int v) {
  switch (v) {
    case static_cast<int>(ChaseStrategy::Default): return u"Default";
    case static_cast<int>(ChaseStrategy::UntilLost): return u"UntilLost";
    case static_cast<int>(ChaseStrategy::StopOnLost): return u"StopOnLost";
  }
  return nullptr;
}

}
