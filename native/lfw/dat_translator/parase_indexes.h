#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

struct ParseIndexesResult {
  bool ok = true;
  std::u16string error;
  Value lists;
};

ParseIndexesResult parase_indexes(const Value& text, const std::u16string& suffix);

}
}
