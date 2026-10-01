#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

struct ResolvePrefabResult {
  bool ok = false;
  bool cycle = false;
  std::vector<std::u16string> chain;
  Value value;
};

ResolvePrefabResult resolve_prefab(const Value& obj, const Value& prefabs);
std::u16string prefab_error_message(const std::u16string& tag, const std::u16string& who,
                                    const std::u16string& what, const ResolvePrefabResult& r);

}
}
