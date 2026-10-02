#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/math/mersenne_twister.h"

namespace lfw {

class Randoming {
 public:
  static MersenneTwister& default_mt();

  Randoming(std::u16string name, std::vector<Value> src, MersenneTwister* mt = nullptr,
            Value duplicate = Value(false));

  static std::shared_ptr<Randoming> create(std::u16string name, std::vector<Value> src,
                                           MersenneTwister* mt = nullptr,
                                           Value duplicate = Value(false));

  Randoming& set_src(std::vector<Value> src);

  Value get();

  const std::u16string& name() const { return _name; }
  MersenneTwister* mt() const { return _mt; }
  const std::vector<Value>& src() const { return _src; }
  const std::vector<Value>& cur() const { return _cur; }
  const Value& taken() const { return _taken; }
  const Value& duplicate() const { return _duplicate; }

 private:
  Value random_get();
  Value random_take();
  double random_in(double l, double r);

  std::u16string _name;
  MersenneTwister* _mt = nullptr;
  std::vector<Value> _src;
  std::vector<Value> _cur;
  Value _taken = Value(NullTag{});
  Value _duplicate = Value(false);
};

}
