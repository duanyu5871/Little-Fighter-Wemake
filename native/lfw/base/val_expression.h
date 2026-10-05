#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/js_num.h"
#include "lfw/core/js_string.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/mersenne_twister.h"

namespace lfw {

// TS: `export type IValGetterFn = (e: Entity) => number;`
template <typename Ctx>
using ValGetterFn = std::function<double(const Ctx&)>;

// Ctx 概念 —— TS 的 `Entity` 在本模块里用到的那两面：
//   * `frame_var(name)`：`e.frame.width / height / centerx / centery`（`DEFAULT_VARS`）
//   * `mt()`：`e.lfw.mt`（`rand` / `pick` / `bag` / `flip` 抽取并写 `mark`）
// 取数器给 `double`（TS 的 `IValGetterFn` 与 `DEFAULT_VARS` 的声明都是 `=> number`）：
// 帧缺字段时 TS 会拿到 `undefined`，那一支（`range(undefined, undefined)` 的提前返回、
// `pick` 的 `v !== undefined` 过滤）不在本模块的覆盖范围内，见 PROTOCOL §6.9.102。
template <typename Ctx>
struct ValExpressionOptions {
  // TS: `options?.tag ?? "val_expr"`
  std::u16string tag;
  // TS: `vars ? {...DEFAULT_VARS, ...vars} : DEFAULT_VARS` —— 同名覆盖默认变量。
  std::map<std::u16string, ValGetterFn<Ctx>> vars;
};

namespace val_expr_detail {

inline bool is_digit(char16_t c) { return c >= u'0' && c <= u'9'; }

inline bool is_ident_char(char16_t c) {
  return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') ||
         c == u'_';
}

// TS 的模块级 `FLIP_VALUES`（`pick` 不改数组 ⇒ 可以共享一份）。
inline const std::vector<double>& flip_values() {
  static const std::vector<double> kValues{-1.0, 1.0};
  return kValues;
}

// `pick` / `flip` 拿到空数组时 TS 返回 `undefined` ⇒ 后续算术会把它变成 `NaN`。
inline double undefined_number() { return std::numeric_limits<double>::quiet_NaN(); }

inline std::u16string dec(std::size_t v) {
  if (v == 0) return u"0";
  std::u16string out;
  while (v > 0) {
    out.insert(out.begin(), static_cast<char16_t>(u'0' + (v % 10)));
    v /= 10;
  }
  return out;
}

}  // namespace val_expr_detail

// TS: `class ValExpression`（`src/LFW/base/ValExpression.ts`）。
//
// 端口照搬「解析期就把表达式编成闭包」的做法：`_get` 就是 TS 的 `private _get`，
// 只是把 `Entity` 换成了模板参数 `Ctx`（照 `base/Expression<Ctx>` 的既有约定）。
// 解析失败不抛异常（lfw 无异常）：`has_err` / `err` 就是 TS 的 `err`，
// `_get` 保持 TS 的回落 `() => 0`，解析停在第一个错误处。
template <typename Ctx>
class ValExpression {
 public:
  ValExpression() = default;

  explicit ValExpression(const std::u16string& source,
                         const ValExpressionOptions<Ctx>& options = {}) {
    tag = options.tag.empty() ? std::u16string(u"val_expr") : options.tag;
    // TS: `(source ?? "").replace(/\s/g, "")`
    for (char16_t c : source) {
      if (!is_str_white_space(c)) text.push_back(c);
    }
    Parse(options.vars);
  }

  double get(const Ctx& e) const { return _get(e); }

  // TS 的三个公开 readonly 字段（`err` 用 `has_err` 表示 `undefined`）。
  std::u16string text;
  std::u16string tag;
  std::u16string err;
  bool has_err = false;

 private:
  using Get = ValGetterFn<Ctx>;

  // TS 的 `DEFAULT_VARS`：`w/h/cx/cy` 读 `e.frame.*`。
  static const std::map<std::u16string, Get>& default_vars() {
    static const std::map<std::u16string, Get> kVars{
        {u"w", [](const Ctx& e) { return e.frame_var(u"width"); }},
        {u"h", [](const Ctx& e) { return e.frame_var(u"height"); }},
        {u"cx", [](const Ctx& e) { return e.frame_var(u"centerx"); }},
        {u"cy", [](const Ctx& e) { return e.frame_var(u"centery"); }},
    };
    return kVars;
  }

  // `text[this.index]`（越界 = JS 的 `undefined`，与任何字符都不相等）。
  std::optional<char16_t> at(std::size_t i) const {
    if (i >= text.size()) return std::nullopt;
    return text[i];
  }

  // TS 的 `ValExpressionError` 收口：记下 `message` 与**当时的** `index`。
  void fail(const std::u16string& message) {
    has_err = true;
    err = u"[ValExpression] " + tag + u": " + message + u" @" + val_expr_detail::dec(_index) +
          u" in \"" + text + u"\"";
  }

  void Parse(const std::map<std::u16string, Get>& custom_vars) {
    _vars = default_vars();
    for (const auto& kv : custom_vars) _vars[kv.first] = kv.second;

    if (text.empty()) {
      fail(u"empty expression");
      return;
    }
    const Get ret = ParseExpr();
    if (has_err) return;
    if (_index < text.size()) {
      fail(u"unexpected '" + text.substr(_index) + u"'");
      return;
    }
    if (ret != nullptr) _get = ret;
  }

  Get ParseExpr() {
    Get ret = ParseTerm();
    if (has_err) return nullptr;
    for (;;) {
      const std::optional<char16_t> op = at(_index);
      if (op != u'+' && op != u'-') return ret;
      ++_index;
      const Get right = ParseTerm();
      if (has_err) return nullptr;
      const Get l = ret;
      const Get r = right;
      // TS 的 `(e) => l(e) + r(e)`：**先左后右**。C++ 的 `l(e) + r(e)` 求值顺序未指定
      // （MSVC 会先算右边）⇒ 必须显式定序，否则两侧的随机数消耗顺序不同。
      ret = *op == u'+' ? Get([l, r](const Ctx& e) {
        const double a = l(e);
        const double b = r(e);
        return a + b;
      })
                        : Get([l, r](const Ctx& e) {
                            const double a = l(e);
                            const double b = r(e);
                            return a - b;
                          });
    }
  }

  Get ParseTerm() {
    Get ret = ParseUnary();
    if (has_err) return nullptr;
    for (;;) {
      const std::optional<char16_t> op = at(_index);
      if (op != u'*' && op != u'/') return ret;
      ++_index;
      const Get right = ParseUnary();
      if (has_err) return nullptr;
      const Get l = ret;
      const Get r = right;
      // 同 `ParseExpr`：定序为「先左后右」。
      ret = *op == u'*' ? Get([l, r](const Ctx& e) {
        const double a = l(e);
        const double b = r(e);
        return a * b;
      })
                        : Get([l, r](const Ctx& e) {
                            const double a = l(e);
                            const double b = r(e);
                            return a / b;
                          });
    }
  }

  Get ParseUnary() {
    const std::optional<char16_t> c = at(_index);
    if (c != u'-') return ParsePrimary();
    ++_index;
    const Get v = ParseUnary();
    if (has_err) return nullptr;
    return Get([v](const Ctx& e) { return -v(e); });
  }

  Get ParsePrimary() {
    const std::optional<char16_t> c = at(_index);
    if (!c.has_value()) {
      fail(u"unexpected end of expression");
      return nullptr;
    }
    if (*c == u'(') {
      ++_index;
      const Get v = ParseExpr();
      if (has_err) return nullptr;
      if (at(_index) != u')') {
        fail(u"missing ')'");
        return nullptr;
      }
      ++_index;
      return v;
    }
    if (val_expr_detail::is_digit(*c) || *c == u'.') return ParseNumber();
    if (val_expr_detail::is_ident_char(*c)) return ParseIdent();
    fail(u"unexpected character '" + std::u16string(1, *c) + u"'");
    return nullptr;
  }

  Get ParseNumber() {
    const std::size_t start = _index;
    for (;;) {
      const std::optional<char16_t> c = at(_index);
      if (!c.has_value() || !val_expr_detail::is_digit(*c)) break;
      ++_index;
    }
    if (at(_index) == u'.') {
      ++_index;
      for (;;) {
        const std::optional<char16_t> c = at(_index);
        if (!c.has_value() || !val_expr_detail::is_digit(*c)) break;
        ++_index;
      }
    }
    const std::u16string raw = text.substr(start, _index - start);
    // TS 的 `Number(raw)` + `Number.isFinite(value)`
    const double value = string_to_number(raw);
    if (!std::isfinite(value)) {
      fail(u"bad number '" + raw + u"'");
      return nullptr;
    }
    return Get([value](const Ctx&) { return value; });
  }

  Get ParseIdent() {
    const std::size_t start = _index;
    for (;;) {
      const std::optional<char16_t> c = at(_index);
      if (!c.has_value() || !val_expr_detail::is_ident_char(*c)) break;
      ++_index;
    }
    const std::u16string name = text.substr(start, _index - start);
    if (at(_index) != u'(') {
      const auto it = _vars.find(name);
      if (it == _vars.end()) {
        fail(u"unknown identifier '" + name + u"'");
        return nullptr;
      }
      return it->second;
    }
    ++_index;
    std::vector<Get> args;
    if (at(_index) != u')') {
      args.push_back(ParseExpr());
      if (has_err) return nullptr;
      while (at(_index) == u',') {
        ++_index;
        args.push_back(ParseExpr());
        if (has_err) return nullptr;
      }
    }
    if (at(_index) != u')') {
      fail(u"missing ')' for '" + name + u"(...'");
      return nullptr;
    }
    ++_index;
    return ParseCall(name, args);
  }

  Get ParseCall(const std::u16string& name, const std::vector<Get>& args) {
    const std::u16string tag_snapshot = tag;
    if (name == u"rand") {
      if (args.size() != 2) {
        fail(u"'rand' expects 2 arguments, got " + val_expr_detail::dec(args.size()));
        return nullptr;
      }
      const Get a = args[0];
      const Get b = args[1];
      return Get([tag_snapshot, a, b](const Ctx& e) -> double {
        MersenneTwister& mt = e.mt();
        // TS: 先写 `mark`、再按**从左到右**求两个参数、最后抽取。
        mt.mark = tag_snapshot;
        const double lo = a(e);
        const double hi = b(e);
        return mt.range(lo, hi);
      });
    }
    if (name == u"pick") {
      if (args.empty()) {
        fail(u"'pick' expects at least 1 argument");
        return nullptr;
      }
      const std::shared_ptr<std::vector<double>> scratch =
          std::make_shared<std::vector<double>>(args.size());
      return Get([tag_snapshot, args, scratch](const Ctx& e) -> double {
        for (std::size_t i = 0; i < args.size(); ++i) (*scratch)[i] = args[i](e);
        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;
        const std::optional<double> v = mt.pick(*scratch);
        return v.has_value() ? *v : val_expr_detail::undefined_number();
      });
    }
    if (name == u"bag") {
      if (args.empty()) {
        fail(u"'bag' expects at least 1 argument");
        return nullptr;
      }
      const std::shared_ptr<std::vector<double>> scratch =
          std::make_shared<std::vector<double>>(args.size());
      // TS 的闭包状态 `cur` / `taken`：抽空后按 `taken` 过滤重填。
      const std::shared_ptr<std::vector<double>> cur = std::make_shared<std::vector<double>>();
      const std::shared_ptr<std::optional<double>> taken = std::make_shared<std::optional<double>>();
      return Get([tag_snapshot, args, scratch, cur, taken](const Ctx& e) -> double {
        for (std::size_t i = 0; i < args.size(); ++i) (*scratch)[i] = args[i](e);
        if (cur->empty()) {
          cur->clear();
          if (args.size() > 1) {
            for (double v : *scratch) {
              if (!taken->has_value() || v != **taken) cur->push_back(v);
            }
          } else {
            *cur = *scratch;
          }
          if (cur->empty()) *cur = *scratch;
        }
        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;
        const double idx = mt.range(0.0, static_cast<double>(cur->size()));
        const std::size_t i = static_cast<std::size_t>(js_to_uint32(idx));
        const double got = (*cur)[i];
        cur->erase(cur->begin() + static_cast<std::ptrdiff_t>(i));
        *taken = got;
        return got;
      });
    }
    if (name == u"flip") {
      if (!args.empty()) {
        fail(u"'flip' expects no arguments");
        return nullptr;
      }
      return Get([tag_snapshot](const Ctx& e) -> double {
        MersenneTwister& mt = e.mt();
        mt.mark = tag_snapshot;
        const std::optional<double> v = mt.pick(val_expr_detail::flip_values());
        return v.has_value() ? *v : val_expr_detail::undefined_number();
      });
    }
    if (name == u"round") {
      if (args.size() != 1) {
        fail(u"'round' expects 1 argument, got " + val_expr_detail::dec(args.size()));
        return nullptr;
      }
      const Get a = args[0];
      return Get([a](const Ctx& e) { return round(a(e)); });
    }
    fail(u"unknown function '" + name + u"'");
    return nullptr;
  }

  std::map<std::u16string, Get> _vars;
  std::size_t _index = 0;
  Get _get = [](const Ctx&) { return 0.0; };
};

}
