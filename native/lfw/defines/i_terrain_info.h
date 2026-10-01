#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class TerrainEnum : int {
  Flat = 0,
  SlopeH = 1,
  SlopeV = 2,
};

inline const std::vector<EnumNumberEntry>& terrain_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Flat", static_cast<double>(TerrainEnum::Flat)},
    {u"SlopeH", static_cast<double>(TerrainEnum::SlopeH)},
    {u"SlopeV", static_cast<double>(TerrainEnum::SlopeV)},
  };
  return e;
}

inline const char16_t* terrain_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(TerrainEnum::Flat): return u"Flat";
    case static_cast<int>(TerrainEnum::SlopeH): return u"SlopeH";
    case static_cast<int>(TerrainEnum::SlopeV): return u"SlopeV";
  }
  return nullptr;
}

}
