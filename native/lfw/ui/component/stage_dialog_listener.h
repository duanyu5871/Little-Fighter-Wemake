#pragma once

#include <functional>

#include "lfw/core/value.h"
#include "lfw/stage/stage.h"

namespace lfw {
namespace ui {

class UIComponent;

// TS `ui/component/StageDialogListener.ts`：盯「换关 + 对白变化」，把 `stage.dialog` 交给 handler。
// 与 TS 的形状差异：
//   * TS 的 `_dialog == dialog` 是对象**同一性**比较（加上 `undefined == null`）⇒ 端口用
//     `same_dialog`（两者都 nullish 或同引用）；对白值恒为对象，不做深比较。
//   * `_dialog` 初值在 TS 是 `undefined` ⇒ 端口用 nullish 的 `Value`。
class StageDialogListener {
 public:
  using Handler = std::function<void(const Value& dialog, const Value& prev)>;

  explicit StageDialogListener(UIComponent& owner, Handler handler = Handler());

  void start();
  void stop();
  StageDialogListener& set_handler(Handler handler);

  void set_stage(stage::Stage* stage);
  void set_dialog(const Value& dialog);

  UIComponent& owner() const { return *_owner; }
  stage::Stage* stage() const { return _stage; }

 protected:
  UIComponent* _owner = nullptr;
  Handler _handler;
  stage::Stage* _stage = nullptr;
  Value _dialog = Value(NullTag{});
  std::function<void()> _world_remover;
  std::function<void()> _stage_remover;
};

}
}
