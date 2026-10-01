#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class ChaseLost : int {
  Hover = 1,
  Leave = 2,
};

inline const std::vector<EnumNumberEntry>& chase_lost_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Hover", static_cast<double>(ChaseLost::Hover)},
    {u"Leave", static_cast<double>(ChaseLost::Leave)},
  };
  return e;
}

inline const char16_t* chase_lost_name_of(int v) {
  switch (v) {
    case static_cast<int>(ChaseLost::Hover): return u"Hover";
    case static_cast<int>(ChaseLost::Leave): return u"Leave";
  }
  return nullptr;
}

}
