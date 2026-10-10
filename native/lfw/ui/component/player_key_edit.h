#pragma once

#include <string>

#include "lfw/lfw.h"
#include "lfw/player_info.h"
#include "lfw/ui/component/player_key_text.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayerKeyEdit.ts`（TAGS = ["PlayerKeyEdit"]）：按键编辑格。
// 继承 `PlayerKeyText`；点父节点聚焦、落键写回玩家并顺延到同玩家的下一个格子。
// TS 里三个回调对象（`p` 父节点 on_click / `l` 键盘 / `r` 指针）在端口分别用
// 父节点回调槽、`KeyListener`、`PointListener` 表达（`l`/`r` 注册进宿主的回调表）。
class PlayerKeyEdit : public PlayerKeyText {
 public:
  static constexpr const char* TAG = "PlayerKeyEdit";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using PlayerKeyText::PlayerKeyText;

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;
  void on_foucs() override;
  void on_blur() override;
  void on_key_changed() override;

 private:
  void on_parent_click(LFWPointerEvent& e);

  struct KeyListener : public LfwKeyboardCallback {
    explicit KeyListener(PlayerKeyEdit* o) : owner(o) {}
    PlayerKeyEdit* owner = nullptr;
    void on_key_down(LfwKeyEvent& e) override;
  };
  struct PointListener : public LfwPointingsCallback {
    explicit PointListener(PlayerKeyEdit* o) : owner(o) {}
    PlayerKeyEdit* owner = nullptr;
    void on_click(LfwPointingEvent& e) override;
  };

  bool _click_me = false;
  KeyListener _key_listener{this};
  PointListener _point_listener{this};
};

}
}
