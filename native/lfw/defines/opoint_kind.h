#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class OpointKind : int {
  Normal = 1,
  Pick = 2,
};

inline const std::vector<EnumNumberEntry>& opoint_kind_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Normal", static_cast<double>(OpointKind::Normal)},
    {u"Pick", static_cast<double>(OpointKind::Pick)},
  };
  return e;
}

inline const char16_t* opoint_kind_name_of(int v) {
  switch (v) {
    case static_cast<int>(OpointKind::Normal): return u"Normal";
    case static_cast<int>(OpointKind::Pick): return u"Pick";
  }
  return nullptr;
}

}
