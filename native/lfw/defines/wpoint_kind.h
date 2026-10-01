#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class WpointKind : int {
  None = 0,
  Bearer = 1,
  Holded = 2,
  Drop = 3,
};

inline const std::vector<EnumNumberEntry>& wpoint_kind_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"None", static_cast<double>(WpointKind::None)},
    {u"Bearer", static_cast<double>(WpointKind::Bearer)},
    {u"Holded", static_cast<double>(WpointKind::Holded)},
    {u"Drop", static_cast<double>(WpointKind::Drop)},
  };
  return e;
}

inline const char16_t* wpoint_kind_name_of(int v) {
  switch (v) {
    case static_cast<int>(WpointKind::None): return u"None";
    case static_cast<int>(WpointKind::Bearer): return u"Bearer";
    case static_cast<int>(WpointKind::Holded): return u"Holded";
    case static_cast<int>(WpointKind::Drop): return u"Drop";
  }
  return nullptr;
}

}
