#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class ArmorEnum : int {
  Defend = 1,
  Fall = 2,
  Times = 3,
  Injury = 4,
};

inline const std::vector<EnumNumberEntry>& armor_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Defend", static_cast<double>(ArmorEnum::Defend)},
    {u"Fall", static_cast<double>(ArmorEnum::Fall)},
    {u"Times", static_cast<double>(ArmorEnum::Times)},
    {u"Injury", static_cast<double>(ArmorEnum::Injury)},
  };
  return e;
}

inline const char16_t* armor_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(ArmorEnum::Defend): return u"Defend";
    case static_cast<int>(ArmorEnum::Fall): return u"Fall";
    case static_cast<int>(ArmorEnum::Times): return u"Times";
    case static_cast<int>(ArmorEnum::Injury): return u"Injury";
  }
  return nullptr;
}

}
