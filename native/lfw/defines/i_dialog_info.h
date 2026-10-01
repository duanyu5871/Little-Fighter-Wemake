#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace dialog_close_by {

inline constexpr const char16_t* kPRESS_A = u"press_a";

}

inline const std::vector<EnumTextEntry>& dialog_close_by_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"PRESS_A", dialog_close_by::kPRESS_A},
  };
  return e;
}

}
