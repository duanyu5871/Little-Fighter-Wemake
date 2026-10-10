#pragma once

#include <vector>

#include "lfw/animation/easing.h"
#include "lfw/animation/sequence.h"
#include "lfw/animation/sine.h"
#include "lfw/base/no_emit_callbacks.h"
#include "lfw/ui/component/flex.h"

namespace lfw {
namespace ui {

class Jalousie;

// TS `IJalousieCallbacks`（`on_anim_end?(v: Jalousie)`）——端口用带 payload 的总线顶替对象参数。
struct JalousieCallbackArgs {
  Jalousie* self = nullptr;
};
using JalousieCallbacks = CallbacksT<JalousieCallbackArgs>;

// TS `ui/component/Jalousie.ts`（TAGS = ["Jalousie"]，继承 `Flex`）：
// `Sequence(Easing(0,0)@250ms, Sine(-1,2,1)@250ms, Easing(1,1)@250ms)` 驱动子节点缩放；
// `open` 读动画方向、写动画 `start`；`click_test` 打开时点击能翻转。
//   * `_anim` 的三个子动画在 TS 里是字段初始化器里的匿名对象 ⇒ 端口用成员 + 指针列接线
//     （声明顺序 = 构造顺序 = `Sequence` 拿到的指针有效）。
class Jalousie : public Flex {
 public:
  static constexpr const char* TAG = "Jalousie";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using Flex::Flex;

  const ClazzTag* clazz() const override { return class_tag(); }

  JalousieCallbacks callbacks;

  Sequence& anim() { return _anim; }
  bool click_test();
  bool open() const;
  void set_open(bool v);
  double w() const;
  double h() const;

  void on_start() override;
  void on_resume() override;
  void on_click(LFWPointerEvent& e) override;
  void on_stop() override;
  void update(double dt) override;
  void on_show() override;

 protected:
  void sync_screen_rect();
  void update_children();

 private:
  // TS 字段初始化器：`new Sequence(new Easing(0,0).set_duration(250), …)` ——
  // 先把子动画时长设好，再由 `Sequence` 的构造器求和（250×3 = 750）并 `start()`。
  static std::vector<Animation*> make_anims(Easing& e1, Sine& s1, Easing& e2);

  Easing _e1{0.0, 0.0};
  Sine _s1{-1.0, 2.0, 1.0};
  Easing _e2{1.0, 1.0};
  Sequence _anim{make_anims(_e1, _s1, _e2)};
};

}
}
