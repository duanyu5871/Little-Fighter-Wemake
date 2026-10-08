#include "lfw/helper/ui_helper.h"

#include <memory>

namespace lfw {
namespace helper {

UIHelper& UIHelper::clear() {
  _all.clear();
  return *this;
}

UIHelper& UIHelper::add(const std::vector<Value>& uis) {
  for (const Value& ui : uis) _all.push_back(ui);
  return *this;
}

void UIHelper::push_page(const std::u16string& id, double stack_idx) {
  auto obj = std::make_shared<Object>();
  obj->set(u"id", Value(id));
  _lfw->push_page(Value(std::move(obj)), stack_idx);
}

void UIHelper::set_page(const std::u16string& id, double stack_idx) {
  auto obj = std::make_shared<Object>();
  obj->set(u"id", Value(id));
  _lfw->set_page(Value(std::move(obj)), stack_idx);
}

}  // namespace helper
}  // namespace lfw
