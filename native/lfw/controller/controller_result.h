#pragma once

#include <functional>
#include <string>
#include <utility>

#include "lfw/core/value.h"

namespace lfw {
namespace controller {

class ControllerResult {
 public:
  using Resolver = std::function<Value(const Value&)>;

  void set_resolver(Resolver fn) { _resolve = std::move(fn); }

  const Value& result() const { return _result; }
  const std::u16string& kind() const { return _kind; }
  const std::u16string& keys() const { return _keys; }
  double time() const { return _time; }

  ControllerResult& clear();
  bool fire(const Value& nf, double time, const std::u16string& keys,
            const std::u16string& kind);
  bool fire2(const Value& result, double time, const std::u16string& keys,
             const std::u16string& kind);

 private:
  Resolver _resolve;
  Value _result;
  std::u16string _kind;
  std::u16string _keys;
  double _time = 0;
};

}
}
