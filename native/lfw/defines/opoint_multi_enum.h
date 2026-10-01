#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class OpointMultiEnum : int {
  AccordingEnemies = 0,
  AccordingAllies = 1,
  Emitter = 2,
};

inline const std::vector<EnumNumberEntry>& opoint_multi_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"AccordingEnemies", static_cast<double>(OpointMultiEnum::AccordingEnemies)},
    {u"AccordingAllies", static_cast<double>(OpointMultiEnum::AccordingAllies)},
    {u"Emitter", static_cast<double>(OpointMultiEnum::Emitter)},
  };
  return e;
}

inline const char16_t* opoint_multi_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(OpointMultiEnum::AccordingEnemies): return u"AccordingEnemies";
    case static_cast<int>(OpointMultiEnum::AccordingAllies): return u"AccordingAllies";
    case static_cast<int>(OpointMultiEnum::Emitter): return u"Emitter";
  }
  return nullptr;
}

}
