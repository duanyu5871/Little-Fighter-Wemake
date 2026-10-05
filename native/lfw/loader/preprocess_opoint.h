#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/val_expression.h"
#include "lfw/core/value.h"

namespace lfw {

// TS 的 `Ditto.warn`（`preprocess_opoint` 的 `compile_gen` 用它报解析错误）。
using ValWarn = std::function<void(const std::u16string&)>;

// TS: `gen_*` → `__gen_*` 的九组字段（`preprocess_opoint` 里九条 `if` 的**原顺序**：
// 告警的顺序、以及差分用例打印的顺序都以它为准）。
inline const std::vector<std::pair<std::u16string, std::u16string>>& opoint_gen_fields() {
  static const std::vector<std::pair<std::u16string, std::u16string>> kFields{
      {u"gen_x", u"__gen_x"},
      {u"gen_y", u"__gen_y"},
      {u"gen_z", u"__gen_z"},
      {u"gen_dvx", u"__gen_dvx"},
      {u"gen_dvy", u"__gen_dvy"},
      {u"gen_dvz", u"__gen_dvz"},
      {u"gen_spread_x", u"__gen_spread_x"},
      {u"gen_spread_y", u"__gen_spread_y"},
      {u"gen_spread_z", u"__gen_spread_z"},
  };
  return kFields;
}

// TS: `preprocess_opoint`（`src/LFW/loader/preprocess_opoint.ts`）。
//
// 把九组 `gen_*`（源字符串）编译成 `__gen_*`（`ValExpression`）。端口不把函数对象塞进
// `Value`（§4.40：`Value` 只有 7 种类型装不下函数），改为把编译成功的项**交还调用方**：
// 键 = `__gen_*` 的全名（`opoint_gen_fields()` 的顺序）。编译失败的项**不在表里** ——
// 与 TS 的 `compile_gen(...) ?? opoint.__gen_x` 同义（该字段保持原值 / 落到 `undefined`）。
// 告警走 `warn`（TS 的 `Ditto.warn(expr.err)`），顺序 = 九条 `if` 的顺序。
//
// ⚠️ TS 的 `(source ?? "").replace(...)` 对**非字符串**的 `gen_*` 会抛 `TypeError`
// （数字 / 对象没有 `replace`）⇒ 端口跳过该字段（这条分支在 TS 侧没有可观察的 trace，
// 记在 PROTOCOL §6.9.102 的「有意不覆盖」里）。
template <typename Ctx>
std::map<std::u16string, ValExpression<Ctx>> preprocess_opoint(const Object& opoint,
                                                               const ValWarn& warn = {}) {
  std::map<std::u16string, ValExpression<Ctx>> out;
  for (const std::pair<std::u16string, std::u16string>& field : opoint_gen_fields()) {
    const Value* src = opoint.get(field.first);
    if (src == nullptr || !truthy(*src)) continue;
    const std::u16string* text = std::get_if<std::u16string>(src);
    if (text == nullptr) continue;
    ValExpressionOptions<Ctx> options;
    options.tag = field.first;
    ValExpression<Ctx> expr(*text, options);
    if (expr.has_err) {
      if (warn) warn(expr.err);
      continue;
    }
    out.emplace(field.second, std::move(expr));
  }
  return out;
}

}
