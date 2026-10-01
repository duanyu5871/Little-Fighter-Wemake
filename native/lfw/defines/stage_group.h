#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace stage_group {

inline constexpr const char16_t* kHidden = u"hidden";
inline constexpr const char16_t* kDev = u"Dev";

}

inline const std::vector<EnumTextEntry>& stage_group_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Hidden", stage_group::kHidden},
    {u"Dev", stage_group::kDev},
  };
  return e;
}

}
