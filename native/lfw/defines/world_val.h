#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace world_val {

inline constexpr const char16_t* ktest = u"test";

}

inline const std::vector<EnumTextEntry>& world_val_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"test", world_val::ktest},
  };
  return e;
}

}
