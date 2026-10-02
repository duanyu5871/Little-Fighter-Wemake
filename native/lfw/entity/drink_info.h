#pragma once

#include "lfw/core/value.h"
#include "lfw/utils/times.h"

namespace lfw {

class DrinkInfo {
 public:
  explicit DrinkInfo(const Value& info);

  Times& hp_h_ticks() { return _hp_h_ticks; }
  const Times& hp_h_ticks() const { return _hp_h_ticks; }

  Times& hp_r_ticks() { return _hp_r_ticks; }
  const Times& hp_r_ticks() const { return _hp_r_ticks; }

  Times& mp_h_ticks() { return _mp_h_ticks; }
  const Times& mp_h_ticks() const { return _mp_h_ticks; }

  const Value& hp_h_value() const { return _hp_h_value; }
  void set_hp_h_value(const Value& v) { _hp_h_value = v; }

  const Value& hp_h_total() const { return _hp_h_total; }
  void set_hp_h_total(const Value& v) { _hp_h_total = v; }

  const Value& hp_h() const { return _hp_h; }
  void set_hp_h(const Value& v) { _hp_h = v; }

  const Value& hp_r_value() const { return _hp_r_value; }
  void set_hp_r_value(const Value& v) { _hp_r_value = v; }

  const Value& hp_r_total() const { return _hp_r_total; }
  void set_hp_r_total(const Value& v) { _hp_r_total = v; }

  const Value& hp_r() const { return _hp_r; }
  void set_hp_r(const Value& v) { _hp_r = v; }

  const Value& mp_h_value() const { return _mp_h_value; }
  void set_mp_h_value(const Value& v) { _mp_h_value = v; }

  const Value& mp_h_total() const { return _mp_h_total; }
  void set_mp_h_total(const Value& v) { _mp_h_total = v; }

  const Value& mp_h() const { return _mp_h; }
  void set_mp_h(const Value& v) { _mp_h = v; }

  bool hp_h_empty() const;
  bool hp_r_empty() const;
  bool mp_h_empty() const;

  Value to_snapshot() const;
  void from_snapshot(const Value& s);

 private:
  Times _hp_h_ticks;
  Value _hp_h_value = Value(0.0);
  Value _hp_h_total = Value(0.0);
  Value _hp_h = Value(0.0);
  Times _hp_r_ticks;
  Value _hp_r_value = Value(0.0);
  Value _hp_r_total = Value(0.0);
  Value _hp_r = Value(0.0);
  Times _mp_h_ticks;
  Value _mp_h_value = Value(0.0);
  Value _mp_h_total = Value(0.0);
  Value _mp_h = Value(0.0);
};

}
