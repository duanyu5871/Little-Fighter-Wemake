#pragma once

#include <memory>
#include <vector>

#include "lfw/utils/math/base.h"

namespace lfw {
namespace stage {

// TS `IJudger<T>` / `IExpression<T>` 里 `Expressions` 用到的那一面。
template <typename T>
class IExpression {
 public:
  virtual ~IExpression() = default;
  virtual bool run(const T& arg) = 0;
};

// TS `Expressions<T>`：按序跑表达式、跑赢了就前进的游标。
template <typename T>
class Expressions {
 public:
  using Items = std::vector<std::shared_ptr<IExpression<T>>>;

  const Items& list() const { return _items; }
  bool is_first() const { return _index <= 0.0; }
  bool is_last() const { return _index >= static_cast<double>(_items.size()) - 1.0; }
  // TS 的 `_index` 是 `protected`（JS 里照样看得见）⇒ 端口给个只读口，供台面观测。
  double index() const { return _index; }

  void reset(const Items& list) {
    _index = 0.0;
    // `if (this._list === list) return;` —— TS 的 `_list` 就是内部那个数组，所以只有
    // 「传进来的正是 `list()` 返回的同一份」才算同一个（不能比地址：临时量可能落在同一格栈上）。
    if (&list == &_items) return;
    _items.clear();
    if (!list.empty()) _items.insert(_items.end(), list.begin(), list.end());
  }

  bool run(const T& arg) {
    const double i = _index;
    if (i < 0.0 || i >= static_cast<double>(_items.size())) return false;
    return _items[static_cast<size_t>(i)]->run(arg);
  }

  void next() { _index = min(_index + 1.0, static_cast<double>(_items.size()) - 1.0); }

  bool flow(const T& arg) {
    bool pass = false;
    do {
      // `is_last` 必须在 `run` 之前取（TS 是先解构再调用）。
      const bool is_last = this->is_last();
      pass = this->run(arg);
      if (!pass || is_last) break;
      this->next();
    } while (true);
    return pass;
  }

 private:
  Items _items;
  double _index = 0.0;
};

}
}
