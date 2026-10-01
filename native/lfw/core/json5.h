#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {

struct Json5Result {
  bool ok = false;
  Value value;
  std::u16string error;
};

struct Json5TextResult {
  bool ok = false;
  std::u16string text;
  std::u16string error;
};

Json5Result json5_parse(const std::u16string& text);

Json5TextResult json5_stringify(const Value& value);

}
