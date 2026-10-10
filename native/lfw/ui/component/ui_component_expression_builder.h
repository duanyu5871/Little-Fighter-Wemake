#pragma once

#include <string>
#include <vector>

namespace lfw {
namespace ui {

// TS `ui/component/UIComponentExpressionBuilder.ts`：拼「`<id>!Tag(args)`」形式的组件表达式串
//（`parse_call_func_expression` 的逆）。
class UIComponentExpressionBuilder {
 public:
  UIComponentExpressionBuilder(const std::u16string& tag,
                               const std::vector<std::u16string>& args);

  UIComponentExpressionBuilder& set_enabled(bool v);
  UIComponentExpressionBuilder& set_id(const std::u16string& id);
  std::u16string to_string() const { return done(); }
  std::u16string done() const;

 private:
  // TS 的 `parts: [id, enabled, TAG, (args)]`。
  std::u16string _id;
  std::u16string _enabled;
  std::u16string _tag;
  std::u16string _args;
};

}
}
