#include "lfw/controller/controller_result.h"

#include <variant>

#include "lfw/utils/type_cast.h"

namespace lfw {
namespace controller {

ControllerResult& ControllerResult::clear() {
  _result = Value();
  _time = 0;
  _keys.clear();
  _kind.clear();
  return *this;
}

bool ControllerResult::fire(const Value& nf, double time,
                            const std::u16string& keys,
                            const std::u16string& kind) {
  const Value result = _resolve ? _resolve(nf) : Value();
  if (!truthy(result)) return false;
  _result = result;
  _time = time;
  _keys = keys;
  _kind = kind;
  return true;
}

bool ControllerResult::fire2(const Value& result, double time,
                             const std::u16string& keys,
                             const std::u16string& kind) {
  _result = result;
  _time = time;
  _keys = keys;
  _kind = kind;
  return true;
}

}
}
