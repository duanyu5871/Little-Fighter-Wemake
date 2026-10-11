#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

// TS `ui/component/CameraCtrl.ts`（TAGS = ["CameraCtrl"]）：按键的相机控制。
class CameraCtrl : public UIComponent {
 public:
  static constexpr const char* TAG = "CameraCtrl";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  // TS `auto`（关键字 ⇒ 端口加尾下划线）。
  bool auto_ = true;
  // TS `candidates?: () => Entity[]`。
  std::function<std::vector<Entity*>()> candidates;

  Entity* staring() const { return _staring; }
  void set_staring(Entity* v);

  void update(double dt) override;
  void on_stop() override;

  void focus_lr(double direction);
  void focus_ud(double direction);

 private:
  Entity* _staring = nullptr;
};

}
}
