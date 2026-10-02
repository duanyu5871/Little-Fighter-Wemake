#include "lfw/controller/controller_key_status.h"

#include <memory>
#include <variant>

#include "lfw/defines/game_key.h"

namespace lfw {
namespace controller {

ControllerKeyStatus::ControllerKeyStatus()
    : L(Value(std::u16string(gk::kL))),
      R(Value(std::u16string(gk::kR))),
      U(Value(std::u16string(gk::kU))),
      D(Value(std::u16string(gk::kD))),
      d(Value(std::u16string(gk::kd))),
      j(Value(std::u16string(gk::kj))),
      a(Value(std::u16string(gk::ka))) {}

KeyStatus* ControllerKeyStatus::slot(const std::u16string& key) {
  const std::vector<KeyStatus*> all = {&L, &R, &U, &D, &d, &j, &a};
  for (KeyStatus* p : all) {
    const Value& k = p->key();
    if (std::holds_alternative<std::u16string>(k) &&
        std::get<std::u16string>(k) == key) {
      return p;
    }
  }
  return nullptr;
}

const KeyStatus* ControllerKeyStatus::slot(const std::u16string& key) const {
  return const_cast<ControllerKeyStatus*>(this)->slot(key);
}

void ControllerKeyStatus::reset() {
  L.reset();
  R.reset();
  U.reset();
  D.reset();
  d.reset();
  j.reset();
  a.reset();
}

Value ControllerKeyStatus::to_snapshot() const {
  Array snap;
  snap.push_back(L.to_snapshot());
  snap.push_back(R.to_snapshot());
  snap.push_back(U.to_snapshot());
  snap.push_back(D.to_snapshot());
  snap.push_back(d.to_snapshot());
  snap.push_back(j.to_snapshot());
  snap.push_back(a.to_snapshot());
  return Value(std::make_shared<Array>(snap));
}

void ControllerKeyStatus::from_snapshot(const Value& s) {
  const Array* arr = as_array(s);
  const size_t n = arr != nullptr ? arr->size() : 0;
  L.from_snapshot(n > 0 ? arr->at(0) : Value());
  R.from_snapshot(n > 1 ? arr->at(1) : Value());
  U.from_snapshot(n > 2 ? arr->at(2) : Value());
  D.from_snapshot(n > 3 ? arr->at(3) : Value());
  d.from_snapshot(n > 4 ? arr->at(4) : Value());
  j.from_snapshot(n > 5 ? arr->at(5) : Value());
  a.from_snapshot(n > 6 ? arr->at(6) : Value());
}

}
}
