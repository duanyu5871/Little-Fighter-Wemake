#pragma once

namespace lfw {

// Mirrors `src/LFW/animation/Loop.ts`：次数/计数（都按 `floor` 取整后 `clamp`）与
// 「一次次播放」的推进。
// ⚠ TS 的方法名 `continue` 是 C++ 关键字 ⇒ 端口叫 `continue_`（语义照抄：推进一次、
// 返回是否成功；`times <= 0` 恒 `true`）。
class Loop {
 public:
  Loop& set_count(double v);
  Loop& set_times(double v);
  Loop& set(double count, double times);
  Loop& reset();
  bool continue_();
  bool done() const;

  double count() const { return _count; }
  double times() const { return _times; }

 private:
  double _times = 1;
  double _count = 0;
};

}
