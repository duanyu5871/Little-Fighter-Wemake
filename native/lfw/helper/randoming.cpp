#include "lfw/helper/randoming.h"

#include <cstddef>
#include <utility>
#include <variant>

#include "lfw/base/clock.h"

namespace lfw {
namespace {

Value duplicate_or_default(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return Value(false);
  return v;
}

}

MersenneTwister& Randoming::default_mt() {
  static MersenneTwister instance(clock() != nullptr ? clock()->now_ms() : 0.0);
  return instance;
}

Randoming::Randoming(std::u16string name, std::vector<Value> src, MersenneTwister* mt,
                     Value duplicate)
    : _name(std::move(name)),
      _mt(mt != nullptr ? mt : &default_mt()),
      _src(std::move(src)),
      _cur(_src),
      _duplicate(duplicate_or_default(duplicate)) {}

std::shared_ptr<Randoming> Randoming::create(std::u16string name, std::vector<Value> src,
                                             MersenneTwister* mt, Value duplicate) {
  return std::make_shared<Randoming>(std::move(name), std::move(src), mt,
                                     duplicate_or_default(duplicate));
}

Randoming& Randoming::set_src(std::vector<Value> src) {
  _src = std::move(src);
  return *this;
}

Value Randoming::get() {
  _taken = truthy(_duplicate) ? random_get() : random_take();
  return _taken;
}

Value Randoming::random_get() {
  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_src.size())));
  if (idx >= _src.size()) return Value();
  return _src[idx];
}

Value Randoming::random_take() {
  if (_cur.empty()) {
    if (_src.size() > 1) {
      std::vector<Value> kept;
      for (const Value& item : _src) {
        if (!equals(item, _taken)) kept.push_back(item);
      }
      _cur = std::move(kept);
    } else {
      _cur = _src;
    }
  }
  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_cur.size())));
  if (idx >= _cur.size()) return Value();
  const Value taken = _cur[idx];
  _cur.erase(_cur.begin() + static_cast<std::ptrdiff_t>(idx));
  return taken;
}

double Randoming::random_in(double l, double r) { return _mt->range(l, r); }

}
