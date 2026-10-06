#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/math/mersenne_twister.h"

namespace lfw {

// `default_mt()` 的实体：TS 里是 `Randoming.mt` 这一个静态实例，**不随元素类型分家**
// （模板静态成员会每个 `T` 一份，所以实例放在这个非模板函数里）。
MersenneTwister& randoming_default_mt();

// TS 的 `taken: T | null = null`、`filter(v => v != this.taken)` 与「抽不到时返回 `undefined`」
// 三处对元素类型的要求。
template <typename T>
struct RandomingItem;

template <>
struct RandomingItem<Value> {
  static Value null_taken() { return Value(NullTag{}); }      // `taken: T | null = null`
  static Value out_of_range() { return Value(); }             // 抽不到 ⇒ `undefined`
  static bool loose_ne(const Value& a, const Value& b) { return !equals(a, b); }
};

template <typename T>
struct RandomingItem<std::shared_ptr<T>> {
  static std::shared_ptr<T> null_taken() { return nullptr; }
  static std::shared_ptr<T> out_of_range() { return nullptr; }
  static bool loose_ne(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b) { return a != b; }
};

// TS `Randoming<T>`。`Value` 版是既有用法（`Randoming` 别名）；`Item` 的
// `Randoming<Randoming<IEntityData>>` 用 `std::shared_ptr<Randoming>` 当元素。
template <typename T>
class RandomingT {
 public:
  using Ptr = std::shared_ptr<RandomingT<T>>;

  static MersenneTwister& default_mt();

  RandomingT(std::u16string name, std::vector<T> src, MersenneTwister* mt = nullptr,
             Value duplicate = Value(false));

  static Ptr create(std::u16string name, std::vector<T> src, MersenneTwister* mt = nullptr,
                    Value duplicate = Value(false));

  RandomingT& set_src(std::vector<T> src);

  T get();

  const std::u16string& name() const { return _name; }
  MersenneTwister* mt() const { return _mt; }
  const std::vector<T>& src() const { return _src; }
  const std::vector<T>& cur() const { return _cur; }
  const T& taken() const { return _taken; }
  const Value& duplicate() const { return _duplicate; }

 private:
  T random_get();
  T random_take();
  double random_in(double l, double r);

  std::u16string _name;
  MersenneTwister* _mt = nullptr;
  std::vector<T> _src;
  std::vector<T> _cur;
  T _taken = RandomingItem<T>::null_taken();
  Value _duplicate = Value(false);
};

using Randoming = RandomingT<Value>;

}
