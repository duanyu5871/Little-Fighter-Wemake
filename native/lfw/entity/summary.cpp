#include "lfw/entity/summary.h"

#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"

namespace lfw {

void Summary::set_picking_sum(const Value& v) {
  const Value o = _picking_sum;
  if (equals(o, v)) return;
  _picking_sum = v;
  callbacks.call(u"on_picking_sum_changed", {v, o, Value()});
}

void Summary::set_damage_sum(const Value& v) {
  const Value o = _damage_sum;
  if (equals(o, v)) return;
  _damage_sum = v;
  callbacks.call(u"on_damage_sum_changed", {v, o, Value()});
}

void Summary::set_kill_sum(const Value& v) {
  const Value o = _kill_sum;
  if (equals(o, v)) return;
  _kill_sum = v;
  callbacks.call(u"on_kill_sum_changed", {v, o, Value()});
}

void Summary::set_hp_lost(const Value& v) {
  const Value o = _hp_lost;
  if (equals(o, v)) return;
  _hp_lost = v;
  callbacks.call(u"on_hp_lost_changed", {v, o, Value()});
}

void Summary::set_mp_usage(const Value& v) {
  const Value o = _mp_usage;
  if (equals(o, v)) return;
  _mp_usage = v;
  callbacks.call(u"on_mp_usage_changed", {v, o, Value()});
}

void Summary::reset(const std::u16string& id) {
  _id = id;
  _damage_sum = Value(0.0);
  _kill_sum = Value(0.0);
  _picking_sum = Value(0.0);
  _hp_lost = Value(0.0);
  _mp_usage = Value(0.0);
}

void Summary::release() {
  callbacks.clear();
  reset(u"");
}

}
