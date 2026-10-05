#pragma once

#include <functional>
#include <utility>

namespace lfw {

// TS `ditto/IClock.ts`：`now()` / `add(handler) -> handle` / `del(handle)` / `hidden()`。
// （4.57 建这个槽时只带了 `now_ms()`，是给 `Randoming::default_mt()` 取种子用的；
// Ticker 要用完整接口 ⇒ 补上 `add`/`del`/`hidden`，并把方法名改回 TS 的 `now`。）
class IClock {
 public:
  virtual ~IClock() = default;
  virtual double now() const = 0;
  virtual int add(std::function<void()> handler) = 0;
  virtual void del(int handle) = 0;
  virtual bool hidden() const = 0;
};

// TS `ditto/ITimeout.ts`：`add(handler, timeout?, ...args) -> id` / `del(id)`。
// `...args` 是 TS 里自己注明的「无用的预留参数」⇒ 不移植。
class ITimeout {
 public:
  virtual ~ITimeout() = default;
  virtual int add(std::function<void()> handler, double timeout) = 0;
  virtual void del(int timer_id) = 0;
};

inline IClock*& clock_slot() {
  static IClock* slot = nullptr;
  return slot;
}

inline IClock* clock() { return clock_slot(); }

inline void set_clock(IClock* value) { clock_slot() = value; }

inline ITimeout*& timeout_slot() {
  static ITimeout* slot = nullptr;
  return slot;
}

inline ITimeout* timeout() { return timeout_slot(); }

inline void set_timeout(ITimeout* value) { timeout_slot() = value; }

// 槽为空时的回落：`Randoming::default_mt()` 在**静态初始化期**取时钟 ⇒ 必须有空的余地
// （与 `callbacks_warn()` 的 `if (warn)` 同款约定）。`add` 回 0 = TS 里「没有句柄」。
inline double clock_now() { return clock() != nullptr ? clock()->now() : 0.0; }

inline bool clock_hidden() { return clock() != nullptr && clock()->hidden(); }

inline int clock_add(std::function<void()> handler) {
  return clock() != nullptr ? clock()->add(std::move(handler)) : 0;
}

inline void clock_del(int handle) {
  if (clock() != nullptr) clock()->del(handle);
}

inline int timeout_add(std::function<void()> handler, double delay) {
  return timeout() != nullptr ? timeout()->add(std::move(handler), delay) : 0;
}

inline void timeout_del(int timer_id) {
  if (timeout() != nullptr) timeout()->del(timer_id);
}

}
