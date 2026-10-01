#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

struct DefinesRuntimeEntry {
  std::u16string name;
  std::u16string value_json5;
  bool is_top_level;
};

const std::vector<DefinesRuntimeEntry>& defines_runtime_entries();

Value bg_info_new();
Value bg_layer_info_new();
Value bg_data_new();

}
