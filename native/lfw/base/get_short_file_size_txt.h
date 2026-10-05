#pragma once

#include <string>

#include "lfw/core/js_string.h"

namespace lfw {

// TS: `base/get_short_file_size_txt.ts`。
//
// 三处 JS 语义要照搬：
//   * `< 1024` 的分支顺序与「整除」都是行为（`bytes /= 1024` 就地改，供下一段判断）；
//   * B 那一支**没有** `toFixed`，直接是 `` `${bytes}B` ``（= JS 的 ToString(number)）；
//   * 另外三支是 `toFixed(1)` 之后 `replace(".0", "")` —— `String.replace` 只替**第一处**
//     （`|x| ≥ 1e21` 走 ToString 时可能出现 ".0" 在指数串里，见 DESIGN §60.4）。
inline std::u16string get_short_file_size_txt(double bytes) {
  const auto strip_dot_zero = [](const std::u16string& s) {
    const size_t at = s.find(u".0");
    if (at == std::u16string::npos) return s;
    return s.substr(0, at) + s.substr(at + 2);
  };
  if (bytes < 1024) return number_to_string(bytes) + u"B";
  bytes /= 1024;
  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"KB";
  bytes /= 1024;
  if (bytes < 1024) return strip_dot_zero(number_to_fixed_1(bytes)) + u"MB";
  bytes /= 1024;
  return strip_dot_zero(number_to_fixed_1(bytes)) + u"GB";
}

}
