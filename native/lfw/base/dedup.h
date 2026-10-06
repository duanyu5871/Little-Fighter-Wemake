#pragma once

#include <string>

namespace lfw {

// Mirrors `src/LFW/base/dedup.ts`：TS 用一个模块级 `Map<string, Promise>` 让**同一 key 的并发
// 调用共享同一次执行**，并在 promise settle 时（`finally`）把条目删掉。
//
// 端口是**同步**的：调用之间没有交错 ⇒ 同一 key 的两次调用不可能重叠，「共享」与「跑两次」在
// 端口里观察不到差别 ⇒ 这里就是直通，每次调用都跑 `body`。`key` 只保留调用形状（不参与逻辑，
// 见 README 偏差表；`Resources` 的差分台面也只做顺序调用，避免引入端口做不到的并发语义）。
template <typename F>
auto deduped(const std::u16string& /*key*/, F&& body) {
  return body();
}

}
