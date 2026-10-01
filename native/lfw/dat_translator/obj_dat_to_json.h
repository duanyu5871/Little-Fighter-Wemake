#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

struct ObjDatToJsonResult {
  bool ok = false;
  std::u16string error;
  Value data;
};

ObjDatToJsonResult obj_dat_to_json(const std::u16string& text, const Value& dat_index);

}
}
