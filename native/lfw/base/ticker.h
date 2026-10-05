#pragma once

#include "lfw/base/clock.h"
#include "lfw/core/value.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

// TS: `export interface ITickerOptions { step_ms(): number; on_step(dt: number): void; }`
class ITickerOptions {
 public:
  virtual ~ITickerOptions() = default;
  virtual double step_ms() = 0;
  virtual void on_step(double dt) = 0;
};

// TS: `class Ticker`（`src/LFW/base/Ticker.ts`）。
//
// 调度器：`Ditto.Clock` / `Ditto.Timeout` 在这里读端口注入的槽（`clock()` / `timeout()`），
// 每处调用都跟 TS 一样**现读**（而不是构造时抓一份），这样换宿主与 TS 换 Ditto 单例同义。
// `_tick` 是 TS 的箭头函数属性（自引用不丢 this）⇒ 端口是成员函数 + `[this]{ tick(); }`。
class Ticker {
 public:
  explicit Ticker(ITickerOptions* options) : _opt(options) {}

  // TS 的公开可调旋钮（差分直接改它们以走到各分支）。
  double safety = 1.06;
  double slew = 0.004;
  double max_span = 2;
  double sleep_threshold = 2;
  double max_lag_steps = 8;
  double rate_window = 1000;
  // 平滑后单步耗时(ms)
  double cost = 0;

  double step() const { return _base * _span; }
  double span() const { return _span; }
  double rate() const { return _rate; }
  bool running() const { return _running; }
  bool pending() const { return _pending; }
  bool paused() const { return _paused; }
  double base() const { return _base; }
  double deadline() const { return _deadline; }
  double last_step() const { return _last_step; }
  int timer_id() const { return _timer; }
  int wake_id() const { return _wake_id; }

  void start() {
    if (_running) return;
    _running = true;
    _pending = false;
    _paused = false;
    _base = _opt->step_ms();
    _span = 1;
    cost = 0;
    const double now = clock_now();
    _deadline = now + _base;
    _last_step = now;
    _rate = _base > 0 ? 1000 / _base : 0;
    _rate_start = now;
    _rate_steps = 0;
    schedule();
  }

  void stop() {
    if (!_running) return;
    _running = false;
    _paused = false;
    cancel();
  }

  // 暂停调度（world.sleep）：期间不再产生任何步进
  void pause() {
    if (_paused) return;
    _paused = true;
    cancel();
  }

  // 恢复调度（world.awake）：距上一帧至少一个步长后才允许步进，避免被数据到达的节奏带快
  void resume() {
    if (!_running || !_paused) return;
    _paused = false;
    _base = _opt->step_ms();
    const double now = clock_now();
    _deadline = max(now, _last_step + _base * _span);
    schedule();
  }

  void resync(bool immediate = false) {
    if (!_running) return;
    const double now = clock_now();
    _base = _opt->step_ms();
    _deadline = immediate ? now : now + _base * _span;
    if (!immediate) _last_step = now;
    _rate_start = now;
    _rate_steps = 0;
    cancel();
    schedule();
  }

 private:
  void cancel() {
    _pending = false;
    if (_timer != 0) {
      timeout_del(_timer);
      _timer = 0;
    }
    if (_wake_id != 0) {
      clock_del(_wake_id);
      _wake_id = 0;
    }
  }

  void schedule() {
    if (!_running || _pending || _paused) return;
    const double delay = _deadline - clock_now();
    if (delay > sleep_threshold && !clock_hidden()) {
      _pending = true;
      _timer = timeout_add([this]() { tick(); }, delay - 1);
      return;
    }
    _pending = true;
    _wake_id = clock_add([this]() { tick(); });
  }

  void tick() {
    _pending = false;
    _timer = 0;
    _wake_id = 0;
    if (!_running) return;
    step_once();
    schedule();
  }

  void step_once() {
    const double base = _opt->step_ms();
    if (base > 0 && abs(base - _base) > base * 0.05) {
      _base = base;
      _span = clamp((cost * safety) / base, 1, max_span);
    } else if (base > 0) {
      _base = base;
    }

    const double t0 = clock_now();
    if (t0 < _deadline) return;

    const double dt = clamp(t0 - _last_step, 0, _base * 4);
    _last_step = t0;
    _opt->on_step(dt);

    const double spent = clock_now() - t0;
    cost = truthy(Value(cost)) ? cost * 0.9 + spent * 0.1 : spent;
    const double want = clamp((cost * safety) / _base, 1, max_span);
    _span = clamp(_span + clamp(want - _span, -slew, slew), 1, max_span);
    _deadline += _base * _span;

    _rate_steps++;
    const double el = t0 - _rate_start;
    if (el >= rate_window) {
      _rate = (_rate_steps * 1000) / el;
      _rate_steps = 0;
      _rate_start = t0;
    }

    const double t1 = clock_now();
    if (t1 - _deadline > _base * max_lag_steps) {
      _deadline = t1 + _base * _span;
      _last_step = t1;
    }
  }

  ITickerOptions* _opt = nullptr;
  bool _running = false;
  bool _pending = false;
  bool _paused = false;
  double _base = 0;
  double _span = 1;
  double _deadline = 0;
  double _last_step = 0;
  int _timer = 0;
  int _wake_id = 0;
  double _rate = 0;
  double _rate_steps = 0;
  double _rate_start = 0;
};

}
