#include "cases.h"

#include <cstdint>
#include <utility>

#include "lfw/core/js_string.h"

namespace lfw {

void Cases::reset() {
  _times = 0;
  _cases.clear();
}

// `push(mark, ...args)`：`args.join()` 是 `Array.join()` 语义（空槽/undefined/null 输出空串）。
void Cases::push(const std::u16string& mark, const std::vector<Value>& args) {
  std::u16string text = mark;
  text.push_back(u'(');
  text += number_to_string(static_cast<double>(++_times));
  text.push_back(u')');
  if (!args.empty()) {
    const Array arr(args);
    text += u":[";
    text += array_join(arr);
    text.push_back(u']');
  }
  _cases.push_back(std::move(text));
}

std::u16string Cases::submit() {
  std::u16string out;
  for (size_t i = 0; i < _cases.size(); ++i) {
    if (i != 0) out += _separator;
    out += _cases[i];
  }
  _cases.clear();
  return out;
}

Cases& mt_cases() {
  static Cases instance(u"mt");
  return instance;
}

}
