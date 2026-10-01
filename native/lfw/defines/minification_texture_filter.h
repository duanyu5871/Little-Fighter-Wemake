#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class MinificationTextureFilter : int {
  Nearest = 1003,
  NearestMipmapNearest = 1004,
  NearestMipmapLinear = 1005,
  Linear = 1006,
  LinearMipmapNearest = 1007,
  LinearMipmapLinear = 1008,
};

inline const std::vector<EnumNumberEntry>& minification_texture_filter_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Nearest", static_cast<double>(MinificationTextureFilter::Nearest)},
    {u"NearestMipmapNearest", static_cast<double>(MinificationTextureFilter::NearestMipmapNearest)},
    {u"NearestMipmapLinear", static_cast<double>(MinificationTextureFilter::NearestMipmapLinear)},
    {u"Linear", static_cast<double>(MinificationTextureFilter::Linear)},
    {u"LinearMipmapNearest", static_cast<double>(MinificationTextureFilter::LinearMipmapNearest)},
    {u"LinearMipmapLinear", static_cast<double>(MinificationTextureFilter::LinearMipmapLinear)},
  };
  return e;
}

inline const char16_t* minification_texture_filter_name_of(int v) {
  switch (v) {
    case static_cast<int>(MinificationTextureFilter::Nearest): return u"Nearest";
    case static_cast<int>(MinificationTextureFilter::NearestMipmapNearest): return u"NearestMipmapNearest";
    case static_cast<int>(MinificationTextureFilter::NearestMipmapLinear): return u"NearestMipmapLinear";
    case static_cast<int>(MinificationTextureFilter::Linear): return u"Linear";
    case static_cast<int>(MinificationTextureFilter::LinearMipmapNearest): return u"LinearMipmapNearest";
    case static_cast<int>(MinificationTextureFilter::LinearMipmapLinear): return u"LinearMipmapLinear";
  }
  return nullptr;
}

}
