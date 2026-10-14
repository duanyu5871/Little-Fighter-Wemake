#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

// `src/LFW/base/Debugging.ts` 的端口。
//
// TS 那边：`IDebugging` 是**结构类型**（只要有 `id` / `__debugging` / `debug` / `warn` /
// `log` 就算数），`make_debugging(obj)` 在对象的构造函数里被调用，把基类上那三个**空转**
// 方法就地换成三条闭包：
//   * 构造时捕获 `cls = Object.getPrototypeOf(obj).constructor`，只为了之后读它的静态
//     `TAG`（`static get TAG() { return this.TAGS[0] }` ⇒ 拿到的永远是**派生类**的标签）；
//   * 调用时现读 `obj.id`（空串/空值 ⇒ 前缀里没有 `<id>` 段）与 `obj.__debugging`；
//   * `debug` 先看 `__debugging`（关 ⇒ 直接 return，连前缀都不拼）；`warn` / `log` **不看**
//     它，永远打；
//   * 前缀 = `[D]` / `[W]` / `[I]` + `<${id}>`（有 id 时）+ `[${cls.TAG}::${func}]`，
//     其后跟上调用时的其余参数，整条交给全局 `Ditto.debug` / `Ditto.warn` / `Ditto.Log`。
//
// 端口形态（两处 JS 专有写法没法直译）：
//   1. JS 能往**实例**上挂方法，C++ 不能 ⇒ `IDebugging` 照 TS 声明三条方法，但实现留在
//      各个实现类里（`UIComponent::debug/warn/log`），它们转调这里的 `debugging_write`
//      （= 三条闭包体）。
//   2. JS 在**构造期**就从原型链拿派生类构造函数，C++ 的虚函数在基类构造期只会派发到基类
//      ⇒ 标签改成调用时经 `debugging_tag()` 取（对同一个实例它不会变，与 TS 的构造期捕获
//      结果等价，见 `UIComponent::debugging_tag()`）。
// `make_debugging(obj)` 仍是 TS 的那个调用点（实现类的构造函数里），负责它在端口里唯一
// 还需要做的事：把对象标成「已接上日志」——没被标过时三条方法保持基类的空转（静默），
// 与 TS 里「还没执行 `make_debugging` 的实例」一致。
//
// 通道 = `ILfwHost`（C++ 侧的全局 `Ditto`：`warn` / `log` / `debug_msg` 三个槽，
// 见 `lfw/lfw.h`；`Ditto.warn` 在端口就是这条缝）。

// TS 三条闭包各自落的 Ditto 方法。
enum class DebugLevel { Debug, Warn, Log };

class IDebugging {
 public:
  virtual ~IDebugging() = default;

  // TS `obj.__debugging`
  virtual bool debugging_enabled() const = 0;
  // TS `obj.id`
  virtual const std::u16string& debugging_id() const = 0;
  // TS `cls.TAG`（`Object.getPrototypeOf(obj).constructor.TAG`）
  virtual const std::u16string& debugging_tag() const = 0;
  // TS 全局 `Ditto.debug` / `Ditto.warn` / `Ditto.Log`
  virtual void debugging_out(DebugLevel level, const std::vector<Value>& args) = 0;

  // TS `IDebugging` 声明的三条方法；`make_debugging` 装上的正是它们的实现
  // （端口里由实现类覆写成 `debugging_write(*this, ...)`，见 `UIComponent`）。
  virtual void debug(const std::u16string& func, const std::vector<Value>& args = {}) = 0;
  virtual void warn(const std::u16string& func, const std::vector<Value>& args = {}) = 0;
  virtual void log(const std::u16string& func, const std::vector<Value>& args = {}) = 0;

  // TS `make_debugging(obj)` 执行之前，实例上的三条日志还是基类的空转实现 ⇒ 静默。
  bool debugging_attached = false;
};

// `src/LFW/base/Debugging.ts:make_debugging(obj)`。
inline void make_debugging(IDebugging& obj) { obj.debugging_attached = true; }

// `make_debugging` 里那三条闭包的共同函数体（拼前缀 + 转发到 `debugging_out`）。
void debugging_write(IDebugging& obj, DebugLevel level, const std::u16string& func,
                     const std::vector<Value>& args);

}
