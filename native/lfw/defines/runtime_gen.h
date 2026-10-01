#pragma once

#include <string>
#include <vector>

namespace lfw {

struct DefinesRuntimeEntry {
  std::u16string name;
  std::u16string value_json5;
  bool is_top_level;
};

const std::vector<DefinesRuntimeEntry>& defines_runtime_entries();

}
