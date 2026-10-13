#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {
class LFW;

namespace helper {

// TS `helper/UIHelper.ts`。
//
// 偏差：TS 的 `add` 会给每个 `id` **动态挂** `push_<id>` / `switch_<id>` 两个方法，
// 端口改为统一的 `push_page(id, stack_idx)` / `set_page(id, stack_idx)`
// （C++ 挂不了动态方法；转发行为一致，只是「方法是否存在」这层没了）。
class UIHelper {
 public:
  static constexpr const char* TAG = "UIHelper";

  explicit UIHelper(LFW& lfw) : _lfw(&lfw) {}

  const std::vector<Value>& all() const { return _all; }

  UIHelper& clear();
  UIHelper& add(const std::vector<Value>& uis);

  void push_page(const std::u16string& id, double stack_idx = 0);
  void set_page(const std::u16string& id, double stack_idx = 0);

 private:
  LFW* _lfw;
  std::vector<Value> _all;
};

}  // namespace helper
}  // namespace lfw
