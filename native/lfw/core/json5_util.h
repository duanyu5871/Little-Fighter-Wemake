#pragma once

#include <cstdint>

#include "lfw/core/json5_unicode.h"

namespace lfw {

inline bool json5_is_space_separator(uint32_t cp) {
  return is_unicode_space_separator(cp);
}

inline bool json5_is_id_start_char(uint32_t cp) {
  if (cp >= u'a' && cp <= u'z') return true;
  if (cp >= u'A' && cp <= u'Z') return true;
  if (cp == u'$' || cp == u'_') return true;
  return is_unicode_id_start(cp);
}

inline bool json5_is_id_continue_char(uint32_t cp) {
  if (cp >= u'a' && cp <= u'z') return true;
  if (cp >= u'A' && cp <= u'Z') return true;
  if (cp >= u'0' && cp <= u'9') return true;
  if (cp == u'$' || cp == u'_') return true;
  if (cp == 0x200C || cp == 0x200D) return true;
  return is_unicode_id_continue(cp);
}

inline bool json5_is_digit(uint32_t cp) { return cp >= u'0' && cp <= u'9'; }

inline bool json5_is_hex_digit(uint32_t cp) {
  if (cp >= u'0' && cp <= u'9') return true;
  if (cp >= u'A' && cp <= u'F') return true;
  if (cp >= u'a' && cp <= u'f') return true;
  return false;
}

}
