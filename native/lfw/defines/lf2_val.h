#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace lf2_val {

inline constexpr const char16_t* ktest2 = u"test2";

}

inline const std::vector<EnumTextEntry>& lf2_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"test2", lf2_val::ktest2},
  };
  return e;
}

}
