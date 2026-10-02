#pragma once

#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"

namespace lfw {

class Summary {
 public:
  explicit Summary(const std::u16string& id) { reset(id); }

  const std::u16string& id() const { return _id; }

  const Value& picking_sum() const { return _picking_sum; }
  void set_picking_sum(const Value& v);

  const Value& damage_sum() const { return _damage_sum; }
  void set_damage_sum(const Value& v);

  const Value& kill_sum() const { return _kill_sum; }
  void set_kill_sum(const Value& v);

  const Value& hp_lost() const { return _hp_lost; }
  void set_hp_lost(const Value& v);

  const Value& mp_usage() const { return _mp_usage; }
  void set_mp_usage(const Value& v);

  void reset(const std::u16string& id);
  void release();

  Callbacks callbacks;

 private:
  std::u16string _id;
  Value _damage_sum;
  Value _picking_sum;
  Value _kill_sum;
  Value _hp_lost;
  Value _mp_usage;
};

}
