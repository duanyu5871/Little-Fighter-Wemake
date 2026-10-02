#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class SeqKeys {
 public:
  SeqKeys(const std::u16string& keys, const Value& data);

  void press(const std::u16string& keys);
  void reset();

  Value to_snapshot() const;
  void from_snapshot(const Value& s);

 private:
  double _idx = 0;
  double _hit = 0;
  std::u16string _keys;
  Value _data;
};

}
}
