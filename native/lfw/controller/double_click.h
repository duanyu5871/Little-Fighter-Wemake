#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class DoubleClick {
 public:
  explicit DoubleClick(const Value& name);

  void press(double time, const Value& data, double interval);
  void step();
  void reset();

  double time() const { return _time; }
  bool fired() const;
  void set_fired(bool v);
  const Value& data(size_t i) const { return _data[i]; }

  Value to_snapshot() const;
  void from_snapshot(const Value& s);

 private:
  Value _data[2];
  double _time = 0;
  Value _used = Value(false);
  Value _fired = Value(false);
  Value _name;
};

}
}
