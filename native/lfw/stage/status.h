#pragma once

#include <string>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace status {

inline constexpr const char16_t* kRunning = u"Running";
inline constexpr const char16_t* kCompleted = u"Completed";
inline constexpr const char16_t* kEnd = u"End";

}

inline const std::vector<EnumTextEntry>& status_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Running", status::kRunning},
    {u"Completed", status::kCompleted},
    {u"End", status::kEnd},
  };
  return e;
}

}
