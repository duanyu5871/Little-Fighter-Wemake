#pragma once

#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace bot {

// `defines/IBotDataSet.ts` 的 `class BotDataSet implements Required<IBotDataSet>`。
//
// TS 侧是一堆同名普通字段 + `static Default`，`BotController.check_bot` 用
// `Object.assign(this.dataset, BotDataSet.Default, bot.dataset)` 灌值：**逐键覆盖、
// 不清旧键**（`Object.assign` 只写源对象自己的可枚举键）。端口因此把值装在一个
// `Object` 里（JS 键序），额外键也能像 TS 一样留下来。
class BotDataSet {
 public:
  BotDataSet();

  static const BotDataSet& Default();

  const Value* get(const std::u16string& key) const;
  double num(const std::u16string& key) const;
  // `Object.assign(this, src)`：只按 `src` 自己的键覆盖（`src` 非对象时什么都不做）。
  void assign(const Value& src);
  // `Object.assign(this, BotDataSet.Default)`：只覆盖 35 个声明序默认键，不清额外键。
  void reset();
  Value dump() const;

 private:
  Value data_;
};

}  // namespace bot
}  // namespace lfw
