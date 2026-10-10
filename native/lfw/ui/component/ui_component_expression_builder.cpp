#include "lfw/ui/component/ui_component_expression_builder.h"

namespace lfw {
namespace ui {

namespace {

// JS `String.prototype.trim` 的常用面（空白集合按 JS 的 \s 简化为空格/制表/换行）。
std::u16string trim(const std::u16string& s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && (s[b] == u' ' || s[b] == u'\t' || s[b] == u'\n' || s[b] == u'\r')) ++b;
  while (e > b && (s[e - 1] == u' ' || s[e - 1] == u'\t' || s[e - 1] == u'\n' || s[e - 1] == u'\r')) {
    --e;
  }
  return s.substr(b, e - b);
}

}

UIComponentExpressionBuilder::UIComponentExpressionBuilder(
    const std::u16string& tag, const std::vector<std::u16string>& args) {
  _tag = tag;
  std::u16string joined;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) joined += u",";
    joined += args[i];
  }
  _args = u"(" + joined + u")";
}

UIComponentExpressionBuilder& UIComponentExpressionBuilder::set_enabled(bool v) {
  _enabled = v ? std::u16string() : std::u16string(u"!");
  return *this;
}

UIComponentExpressionBuilder& UIComponentExpressionBuilder::set_id(const std::u16string& id) {
  const std::u16string t = trim(id);
  _id = t.empty() ? std::u16string() : u"<" + t + u">";
  return *this;
}

std::u16string UIComponentExpressionBuilder::done() const {
  return _id + _enabled + _tag + _args;
}

}
}
