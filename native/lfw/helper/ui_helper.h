#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace helper {

// `lfw.layers.push_page(page, stack_idx)` / `set_page(page, stack_idx)`（UI 层未移植 ⇒ 缝）。
class IUiHelperLfw {
 public:
  virtual ~IUiHelperLfw() = default;
  virtual void push_page(const Value& page, double stack_idx) = 0;
  virtual void set_page(const Value& page, double stack_idx) = 0;
};

// TS `helper/UIHelper.ts`。
//
// 偏差：TS 的 `add` 会给每个 `id` **动态挂** `push_<id>` / `switch_<id>` 两个方法，
// 端口改为统一的 `push_page(id, stack_idx)` / `set_page(id, stack_idx)`
// （C++ 挂不了动态方法；转发行为一致，只是「方法是否存在」这层没了）。
class UIHelper {
 public:
  static constexpr const char* TAG = "UIHelper";

  explicit UIHelper(IUiHelperLfw& lfw) : _lfw(&lfw) {}

  const std::vector<Value>& all() const { return _all; }

  UIHelper& clear();
  UIHelper& add(const std::vector<Value>& uis);

  void push_page(const std::u16string& id, double stack_idx = 0);
  void set_page(const std::u16string& id, double stack_idx = 0);

 private:
  IUiHelperLfw* _lfw;
  std::vector<Value> _all;
};

}  // namespace helper
}  // namespace lfw
