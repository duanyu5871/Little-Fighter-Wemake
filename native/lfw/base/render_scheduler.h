#pragma once

#include <functional>

namespace lfw {

// TS `ditto/IRender.ts`（`Ditto.Render`）：**定时渲染**的调度槽。
//
// 与 `IClock`（`Ditto.Clock`）的区别很重要：`Ditto.Clock.add` 是**一次性**的（`Clock.ts` 的
// `flush` 把待发批次跑一遍就清空），`Ticker` 正是按这个语义写的（每次 `tick` 重新
// `schedule`）；而 `Ditto.Render.add` 是 raf 式的**重复**回调（`DittoImpl/Render.ts` 在回调里
// 再 `requestAnimationFrame`），`World::start_render` 的渲染循环只 `add` 一次、之后靠它每帧
// 回调 ⇒ 两者**不能共用一个槽**（4J 一开始把渲染也接在时钟槽上，被测出「`Ticker` 停掉后
// 仍有旧回调」的 use-after-free）。
class IRenderScheduler {
 public:
  virtual ~IRenderScheduler() = default;
  virtual int add(std::function<void()> handler) = 0;
  virtual void del(int handle) = 0;
};

inline IRenderScheduler*& render_scheduler_slot() {
  static IRenderScheduler* slot = nullptr;
  return slot;
}

inline IRenderScheduler* render_scheduler() { return render_scheduler_slot(); }

inline void set_render_scheduler(IRenderScheduler* value) { render_scheduler_slot() = value; }

// 槽为空时回 0（TS 里 `Ditto.Render` 必定装好；端口沿用 `clock_add` 的既有约定）。
inline int render_add(std::function<void()> handler) {
  return render_scheduler() != nullptr ? render_scheduler()->add(std::move(handler)) : 0;
}

inline void render_del(int handle) {
  if (render_scheduler() != nullptr) render_scheduler()->del(handle);
}

}
