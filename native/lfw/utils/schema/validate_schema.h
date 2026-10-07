#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace schema {

// Mirrors `src/LFW/utils/schema/validate_schema.ts`（`SchemaValidator`）。
//
// 形状差异：
//   * TS 里 schema 是 `ISchema` 接口对象；端口把它当 **Value**（生成器 dump 的纯数据表，
//     见 `defines/schemas_gen.h`）——校验里对「对象/数组/字符串」的操作全部照抄。
//   * TS 的 `instance_getter` / `instance_setter` 钩子、以及类类型（`type` 是构造器/自定义类）
//     的 `Object.defineProperty` 惰性属性分支**未建形**：C++ 的 `Value` 装不下函数，已移植的
//     10 份 schema 也没有这种形态（schema 表是纯数据）。switch 的 default 分支同理
//     （「type 是函数 ⇒ 值必须是字符串」的判定落不了地）。
//   * TS `errors` / `warnings` 是 protected 数组 + getter；端口同形（私有数组 + 访问器）。
//   * `validate` 会**就地**改 `value`（数组项/对象属性的浅拷贝写回、嵌套递归）——照抄。
class SchemaValidator {
 public:
  static SchemaValidator& Default();

  const std::vector<std::u16string>& errors() const { return _errors; }
  const std::vector<std::u16string>& warnings() const { return _warnings; }

  bool validate(const Value& value, const Value& schema);
  void reset();

 private:
  std::vector<std::u16string> _errors;
  std::vector<std::u16string> _warnings;
};

}
}
