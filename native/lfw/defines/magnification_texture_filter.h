#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class MagnificationTextureFilter : int {
  Nearest = 1003,
  Linear = 1006,
};

inline const std::vector<EnumNumberEntry>& magnification_texture_filter_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Nearest", static_cast<double>(MagnificationTextureFilter::Nearest)},
    {u"Linear", static_cast<double>(MagnificationTextureFilter::Linear)},
  };
  return e;
}

inline const char16_t* magnification_texture_filter_name_of(int v) {
  switch (v) {
    case static_cast<int>(MagnificationTextureFilter::Nearest): return u"Nearest";
    case static_cast<int>(MagnificationTextureFilter::Linear): return u"Linear";
  }
  return nullptr;
}

}
