#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// `Cases`（`src/LFW/Cases.ts`）：调试用例收集器。
// `push` 的文本格式与 TS 逐字节一致：`<mark>(<times>)` 或 `<mark>(<times>):[a,b,c]`
// （参数用 `Array.join()` 语义拼接，`undefined`/`null` 变成空串）；
// `submit()` 用 `separator` 连接全部条目后清空 `cases`（`times` 不清零）。
class Cases {
 public:
  explicit Cases(std::u16string name) : _name(std::move(name)) {}

  const std::u16string& name() const { return _name; }
  const std::u16string& separator() const { return _separator; }
  const std::vector<std::u16string>& cases() const { return _cases; }

  void reset();
  void push(const std::u16string& mark, const std::vector<Value>& args);
  std::u16string submit();

 private:
  std::u16string _name;
  // `'￥'`：直接写 escaped 形式，避免依赖源文件编码。
  std::u16string _separator = u"\uffe5";
  std::vector<std::u16string> _cases;
  uint64_t _times = 0;
};

// `cases_instances.ts` 的 `mt_cases` / `sus_cases`。
Cases& mt_cases();
Cases& sus_cases();

}
