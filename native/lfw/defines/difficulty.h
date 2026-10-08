#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class Difficulty : int {
  Easy = 1,
  Normal = 2,
  Difficult = 3,
  Crazy = 4,
  MIN = 1,
  MAX = 4,
};

inline const std::vector<EnumNumberEntry>& difficulty_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Easy", static_cast<double>(Difficulty::Easy)},
    {u"Normal", static_cast<double>(Difficulty::Normal)},
    {u"Difficult", static_cast<double>(Difficulty::Difficult)},
    {u"Crazy", static_cast<double>(Difficulty::Crazy)},
    {u"MIN", static_cast<double>(Difficulty::MIN)},
    {u"MAX", static_cast<double>(Difficulty::MAX)},
  };
  return e;
}

inline const char16_t* difficulty_name_of(int v) {
  switch (v) {
    case static_cast<int>(Difficulty::MIN): return u"MIN";
    case static_cast<int>(Difficulty::Normal): return u"Normal";
    case static_cast<int>(Difficulty::Difficult): return u"Difficult";
    case static_cast<int>(Difficulty::MAX): return u"MAX";
  }
  return nullptr;
}

// `Defines.DifficultyLabels`（TS 是枚举值→文案键的表）。
inline std::u16string difficulty_label(double v) {
  switch (static_cast<int>(v)) {
    case static_cast<int>(Difficulty::Easy): return u"easy";
    case static_cast<int>(Difficulty::Normal): return u"normal";
    case static_cast<int>(Difficulty::Difficult): return u"difficult";
    case static_cast<int>(Difficulty::Crazy): return u"crazy";
  }
  return std::u16string();
}

}
