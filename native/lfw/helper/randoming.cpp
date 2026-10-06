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

MersenneTwister& randoming_default_mt() {
  static MersenneTwister instance(clock_now());
  return instance;
}

template <typename T>
MersenneTwister& RandomingT<T>::default_mt() {
  return randoming_default_mt();
}

template <typename T>
RandomingT<T>::RandomingT(std::u16string name, std::vector<T> src, MersenneTwister* mt,
                          Value duplicate)
    : _name(std::move(name)),
      _mt(mt != nullptr ? mt : &default_mt()),
      _src(std::move(src)),
      _cur(_src),
      _duplicate(duplicate_or_default(duplicate)) {}

template <typename T>
typename RandomingT<T>::Ptr RandomingT<T>::create(std::u16string name, std::vector<T> src,
                                                  MersenneTwister* mt, Value duplicate) {
  return std::make_shared<RandomingT<T>>(std::move(name), std::move(src), mt,
                                         duplicate_or_default(duplicate));
}

template <typename T>
RandomingT<T>& RandomingT<T>::set_src(std::vector<T> src) {
  _src = std::move(src);
  return *this;
}

template <typename T>
T RandomingT<T>::get() {
  _taken = truthy(_duplicate) ? random_get() : random_take();
  return _taken;
}

template <typename T>
T RandomingT<T>::random_get() {
  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_src.size())));
  if (idx >= _src.size()) return RandomingItem<T>::out_of_range();
  return _src[idx];
}

template <typename T>
T RandomingT<T>::random_take() {
  if (_cur.empty()) {
    if (_src.size() > 1) {
      std::vector<T> kept;
      for (const T& item : _src) {
        if (RandomingItem<T>::loose_ne(item, _taken)) kept.push_back(item);
      }
      _cur = std::move(kept);
    } else {
      _cur = _src;
    }
  }
  const size_t idx = static_cast<size_t>(random_in(0.0, static_cast<double>(_cur.size())));
  if (idx >= _cur.size()) return RandomingItem<T>::out_of_range();
  const T taken = _cur[idx];
  _cur.erase(_cur.begin() + static_cast<std::ptrdiff_t>(idx));
  return taken;
}

template <typename T>
double RandomingT<T>::random_in(double l, double r) {
  _mt->mark = _name;
  return _mt->range(l, r);
}

template class RandomingT<Value>;
template class RandomingT<std::shared_ptr<Randoming>>;

}
