#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class KeyStatus {
 public:
  explicit KeyStatus(const Value& key);

  double use();
  bool is_start(double time) const;
  bool is_hit(double time, double key_hit_duration) const;
  bool is_hld(double time, double key_hit_duration) const;
  bool is_end() const;

  void hit(const Value& t, double time);
  void end(double time);
  void reset();

  Value to_snapshot() const;
  void from_snapshot(const Value& s);

 private:
  double _d_time = 0;
  double _u_time = 0;
  double _used = 0;
  Value _key;
};

}
}
