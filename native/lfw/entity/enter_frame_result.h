#pragma once

#include <cstdint>

namespace lfw {

// Mirrors `src/LFW/entity/EnterFrameResult.ts`.  The numeric values are the TS ones
// (`Gone` 0 / `NotFound` 1 / `Entered` 2 / `Fallback` 3) so a trace can print them.
enum class EnterFrameResult : int {
  Gone = 0,
  NotFound = 1,
  Entered = 2,
  Fallback = 3,
};

inline const char16_t* enter_frame_result_name(EnterFrameResult r) {
  switch (r) {
    case EnterFrameResult::Gone:
      return u"Gone";
    case EnterFrameResult::NotFound:
      return u"NotFound";
    case EnterFrameResult::Entered:
      return u"Entered";
    case EnterFrameResult::Fallback:
      return u"Fallback";
  }
  return u"";
}

}
