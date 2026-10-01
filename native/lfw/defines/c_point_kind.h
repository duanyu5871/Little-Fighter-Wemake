#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class CPointKind : int {
  Attacker = 1,
  Victim = 2,
};

inline const std::vector<EnumNumberEntry>& c_point_kind_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Attacker", static_cast<double>(CPointKind::Attacker)},
    {u"Victim", static_cast<double>(CPointKind::Victim)},
  };
  return e;
}

inline const char16_t* c_point_kind_name_of(int v) {
  switch (v) {
    case static_cast<int>(CPointKind::Attacker): return u"Attacker";
    case static_cast<int>(CPointKind::Victim): return u"Victim";
  }
  return nullptr;
}

}
