#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class BdyKind : int {
  Normal = 0,
  Criminal = 1,
  Defend = 2000,
  Ignore = 10000,
};

inline const std::vector<EnumNumberEntry>& bdy_kind_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Normal", static_cast<double>(BdyKind::Normal)},
    {u"Criminal", static_cast<double>(BdyKind::Criminal)},
    {u"Defend", static_cast<double>(BdyKind::Defend)},
    {u"Ignore", static_cast<double>(BdyKind::Ignore)},
  };
  return e;
}

inline const char16_t* bdy_kind_name_of(int v) {
  switch (v) {
    case static_cast<int>(BdyKind::Normal): return u"Normal";
    case static_cast<int>(BdyKind::Criminal): return u"Criminal";
    case static_cast<int>(BdyKind::Defend): return u"Defend";
    case static_cast<int>(BdyKind::Ignore): return u"Ignore";
  }
  return nullptr;
}

}
