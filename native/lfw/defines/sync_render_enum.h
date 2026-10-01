#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class SyncRenderEnum : int {
  Unlimited = 0,
  Half = 1,
  Sync = 2,
  FPS_60 = 3,
  FPS_120 = 4,
};

inline const std::vector<EnumNumberEntry>& sync_render_enum_entries() {
  static const std::vector<EnumNumberEntry> e = {
    {u"Unlimited", static_cast<double>(SyncRenderEnum::Unlimited)},
    {u"Half", static_cast<double>(SyncRenderEnum::Half)},
    {u"Sync", static_cast<double>(SyncRenderEnum::Sync)},
    {u"FPS_60", static_cast<double>(SyncRenderEnum::FPS_60)},
    {u"FPS_120", static_cast<double>(SyncRenderEnum::FPS_120)},
  };
  return e;
}

inline const char16_t* sync_render_enum_name_of(int v) {
  switch (v) {
    case static_cast<int>(SyncRenderEnum::Unlimited): return u"Unlimited";
    case static_cast<int>(SyncRenderEnum::Half): return u"Half";
    case static_cast<int>(SyncRenderEnum::Sync): return u"Sync";
    case static_cast<int>(SyncRenderEnum::FPS_60): return u"FPS_60";
    case static_cast<int>(SyncRenderEnum::FPS_120): return u"FPS_120";
  }
  return nullptr;
}

}
