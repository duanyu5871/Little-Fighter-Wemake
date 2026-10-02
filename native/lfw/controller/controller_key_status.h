#pragma once

#include <string>

#include "lfw/controller/key_status.h"
#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class ControllerKeyStatus {
 public:
  ControllerKeyStatus();

  KeyStatus* slot(const std::u16string& key);
  const KeyStatus* slot(const std::u16string& key) const;

  void reset();

  Value to_snapshot() const;
  void from_snapshot(const Value& s);

  KeyStatus L;
  KeyStatus R;
  KeyStatus U;
  KeyStatus D;
  KeyStatus d;
  KeyStatus j;
  KeyStatus a;
};

}
}
