#include "lfw/controller/double_click.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/type_cast.h"

namespace lfw {
namespace controller {

DoubleClick::DoubleClick(const Value& name) : _name(name) {}

void DoubleClick::press(double time, const Value& data, double interval) {
  if (_time + time <= interval) {
    _time = time;
    _data[1] = data;
    _fired = Value(true);
    return;
  }
  _time = -time;
  _data[0] = data;
  _data[1] = Value();
}

void DoubleClick::step() {
  _time = -_time;
  _data[0] = _data[1];
  _data[1] = Value();
  _fired = Value(false);
}

void DoubleClick::reset() {
  _time = 0;
  _data[0] = Value();
  _data[1] = Value();
  _fired = Value(false);
  _used = Value(false);
}

Value DoubleClick::to_snapshot() const {
  Array pair;
  pair.push_back(_data[0]);
  pair.push_back(_data[1]);
  Object o;
  o.set(u"data", Value(std::make_shared<Array>(pair)));
  o.set(u"time", Value(_time));
  o.set(u"used", _used);
  o.set(u"fired", _fired);
  o.set(u"name", _name);
  return Value(std::make_shared<Object>(o));
}

void DoubleClick::from_snapshot(const Value& s) {
  const Value pair = field_or(s, u"data");
  if (const Array* a = as_array(pair)) {
    _data[0] = a->size() > 0 ? a->at(0) : Value();
    _data[1] = a->size() > 1 ? a->at(1) : Value();
  } else {
    _data[0] = Value();
    _data[1] = Value();
  }
  _time = to_number(field_or(s, u"time"));
  _used = field_or(s, u"used");
  _fired = field_or(s, u"fired");
  _name = field_or(s, u"name");
}

bool DoubleClick::fired() const { return truthy(_fired); }

void DoubleClick::set_fired(bool v) { _fired = Value(v); }

}
}
