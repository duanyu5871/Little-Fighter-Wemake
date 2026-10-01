#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class TextureWrapping : int {
  Repeat = 1000,
  ClampToEdge = 1001,
  MirroredRepeat = 1002,
};

inline const std::vector<EnumNumberEntry>& texture_wrapping_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Repeat", static_cast<double>(TextureWrapping::Repeat)},
    {u"ClampToEdge", static_cast<double>(TextureWrapping::ClampToEdge)},
    {u"MirroredRepeat", static_cast<double>(TextureWrapping::MirroredRepeat)},
  };
  return e;
}

inline const char16_t* texture_wrapping_name_of(int v) {
  switch (v) {
    case static_cast<int>(TextureWrapping::Repeat): return u"Repeat";
    case static_cast<int>(TextureWrapping::ClampToEdge): return u"ClampToEdge";
    case static_cast<int>(TextureWrapping::MirroredRepeat): return u"MirroredRepeat";
  }
  return nullptr;
}

}
