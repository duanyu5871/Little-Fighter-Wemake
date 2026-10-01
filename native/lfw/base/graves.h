#pragma once

#include <optional>
#include <vector>

namespace lfw {

template <typename T>
class Graves {
 public:
  void add(const T& t) {
    if (_i == 0) {
      _l.push_back(t);
    } else {
      _l[--_i] = t;
    }
  }

  std::optional<T> take() {
    if (_i >= _l.size()) return std::nullopt;
    const std::optional<T> ret = _l[_i];
    if (!ret.has_value()) return std::nullopt;
    _l[_i] = std::nullopt;
    ++_i;
    return ret;
  }

  const std::vector<std::optional<T>>& l() const { return _l; }

 private:
  std::vector<std::optional<T>> _l;
  size_t _i = 0;
};

}
