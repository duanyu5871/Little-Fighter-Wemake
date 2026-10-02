#pragma once

#include <string>

#include "lfw/controller/double_click.h"
#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class ControllerDoubleClicks {
 public:
  ControllerDoubleClicks();

  void reset();
  Value to_snapshot() const;
  void from_snapshot(const Value& s);

  DoubleClick* slot(const std::u16string& key);

 private:
  DoubleClick L;
  DoubleClick R;
  DoubleClick U;
  DoubleClick D;
  DoubleClick d;
  DoubleClick j;
  DoubleClick a;
};

}
}
