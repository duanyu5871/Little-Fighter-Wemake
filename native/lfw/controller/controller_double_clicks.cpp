#include "lfw/controller/controller_double_clicks.h"

#include <cstddef>
#include <memory>
#include <string>

#include "lfw/controller/double_click.h"
#include "lfw/core/value.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace controller {

ControllerDoubleClicks::ControllerDoubleClicks()
    : L(Value(std::u16string(u"d"))),
      R(Value(std::u16string(u"a"))),
      U(Value(std::u16string(u"j"))),
      D(Value(std::u16string(u"L"))),
      d(Value(std::u16string(u"R"))),
      j(Value(std::u16string(u"U"))),
      a(Value(std::u16string(u"D"))) {}

DoubleClick* ControllerDoubleClicks::slot(const std::u16string& key) {
  if (key == u"L") return &L;
  if (key == u"R") return &R;
  if (key == u"U") return &U;
  if (key == u"D") return &D;
  if (key == u"d") return &d;
  if (key == u"j") return &j;
  if (key == u"a") return &a;
  return nullptr;
}

void ControllerDoubleClicks::reset() {
  DoubleClick* const all[] = {&L, &R, &U, &D, &d, &j, &a};
  for (DoubleClick* one : all) one->reset();
}

Value ControllerDoubleClicks::to_snapshot() const {
  Object o;
  o.set(u"L", L.to_snapshot());
  o.set(u"R", R.to_snapshot());
  o.set(u"U", U.to_snapshot());
  o.set(u"D", D.to_snapshot());
  o.set(u"d", d.to_snapshot());
  o.set(u"j", j.to_snapshot());
  o.set(u"a", a.to_snapshot());
  return Value(std::make_shared<Object>(o));
}

void ControllerDoubleClicks::from_snapshot(const Value& s) {
  DoubleClick* const targets[] = {&L, &R, &U, &D, &d, &j, &a};
  const char16_t* const kKeys[] = {u"L", u"R", u"U", u"D", u"d", u"j", u"a"};
  for (size_t i = 0; i < 7; ++i) {
    const Value item = field_or(s, kKeys[i]);
    if (as_object(item) != nullptr) targets[i]->from_snapshot(item);
  }
}

}
}
