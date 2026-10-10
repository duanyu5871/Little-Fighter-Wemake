#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "lfw/ui/component/fade_out_opacity.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/LoadingContentText.ts`（TAGS = ["LoadingContentText"]）：
// args = [page?, fade_out_duration?, fade_out_delay?]；订 `lfw.callbacks` 的加载四事件。
// `init` 把 `FadeOutOpacity.expression(duration, delay)` 解析成组件描述、经 factory 建出来挂到自己的节点上。
class LoadingContentText : public UIComponent {
 public:
  static constexpr const char* TAG = "LoadingContentText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  // TS `get fade_out_duration` / `get fade_out_delay`：`num(1) ?? 0` / `num(2) ?? 0`。
  double fade_out_duration();
  double fade_out_delay();

  void init() override;
  void on_resume() override;
  void on_pause() override;

  void on_loading_start();
  void on_loading_end();
  void on_progress(const std::u16string& text, double progress);
  void on_loading_failed(const Value& reason);
  // TS 模块函数 `format_reason(reason)`（端口：宿主给的一律是字符串/普通值，没有 Error 实体）。
  std::u16string format_reason(const Value& reason);

 protected:
  FadeOutOpacity* fadeout = nullptr;
  std::unique_ptr<UIComponent> _fadeout_owner;
  std::optional<std::u16string> failed_reason;
  std::function<void()> _remover;
};

}
}
