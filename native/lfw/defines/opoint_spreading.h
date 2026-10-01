#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class OpointSpreading : int {
  Normal = 0,
  Spreading = 1,
  FloatRange = 2,
};

inline const std::vector<EnumNumberEntry>& opoint_spreading_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Normal", static_cast<double>(OpointSpreading::Normal)},
    {u"Spreading", static_cast<double>(OpointSpreading::Spreading)},
    {u"FloatRange", static_cast<double>(OpointSpreading::FloatRange)},
  };
  return e;
}

inline const char16_t* opoint_spreading_name_of(int v) {
  switch (v) {
    case static_cast<int>(OpointSpreading::Normal): return u"Normal";
    case static_cast<int>(OpointSpreading::Spreading): return u"Spreading";
    case static_cast<int>(OpointSpreading::FloatRange): return u"FloatRange";
  }
  return nullptr;
}

}
