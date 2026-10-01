#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace cheat_enum {

inline constexpr const char16_t* kLF2_NET = u"LF2_NET";
inline constexpr const char16_t* kHERO_FT = u"HERO_FT";
inline constexpr const char16_t* kGIM_INK = u"GIM_INK";

}

inline const std::vector<EnumTextEntry>& cheat_enum_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"LF2_NET", cheat_enum::kLF2_NET},
    {u"HERO_FT", cheat_enum::kHERO_FT},
    {u"GIM_INK", cheat_enum::kGIM_INK},
  };
  return e;
}

}
