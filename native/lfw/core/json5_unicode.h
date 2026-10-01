#pragma once

#include <cstddef>
#include <cstdint>

namespace lfw {

struct CpRange {
  uint32_t lo;
  uint32_t hi;
};

extern const CpRange kSpaceSeparator[];
extern const size_t kSpaceSeparatorSize;

extern const CpRange kIdStart[];
extern const size_t kIdStartSize;

extern const CpRange kIdContinue[];
extern const size_t kIdContinueSize;

inline bool in_table(const CpRange* t, size_t n, uint32_t cp) {
  size_t lo = 0;
  size_t hi = n;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (cp < t[mid].lo) {
      hi = mid;
    } else if (cp > t[mid].hi) {
      lo = mid + 1;
    } else {
      return true;
    }
  }
  return false;
}

inline bool is_unicode_space_separator(uint32_t cp) {
  return in_table(kSpaceSeparator, kSpaceSeparatorSize, cp);
}

inline bool is_unicode_id_start(uint32_t cp) {
  return in_table(kIdStart, kIdStartSize, cp);
}

inline bool is_unicode_id_continue(uint32_t cp) {
  return in_table(kIdContinue, kIdContinueSize, cp);
}

}
