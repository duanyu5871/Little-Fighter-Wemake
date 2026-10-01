#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace background_group {

inline constexpr const char16_t* kRegular = u"regular";
inline constexpr const char16_t* kHidden = u"hidden";

}

inline const std::vector<EnumTextEntry>& background_group_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Regular", background_group::kRegular},
    {u"Hidden", background_group::kHidden},
  };
  return e;
}

}
