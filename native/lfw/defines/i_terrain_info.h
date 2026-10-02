#pragma once

#include <string>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class TerrainEnum : int {
  Flat = 0,
  SlopeH = 1,
  SlopeV = 2,
};

struct ITerrainInfo {
  std::u16string id;
  std::u16string name;
  int type = 0;
  double x1 = 0.0;
  double x2 = 0.0;
  double z1 = 0.0;
  double z2 = 0.0;
  double h1 = 0.0;
  double h2 = 0.0;
};

inline ITerrainInfo terrain_info_new() { return ITerrainInfo{}; }

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
