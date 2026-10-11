#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {
namespace ui {

class ArrowSwitcher;
class SliderHandle;

// TS `ui/component/MiscSettingsLogic.ts`（TAGS = ["MiscSettingsLogic"]）：杂项设置页的逻辑
// （BGM/音效音量与开关、渲染频率、UPS、语言、队伍描边、统计开关）。
//
// TS 的 `_anys.a..j`（`||=` 惰性缓存）端口化成 10 个槽位指针（`nullptr` 视为未命中，
// 下次重查 —— 与 `undefined ||= x` 每次重求值同口径）。
class MiscSettingsLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "MiscSettingsLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using UIComponent::UIComponent;

  const ClazzTag* clazz() const override { return class_tag(); }

  SliderHandle* bgm_toggle();     // a
  SliderHandle* bgm_volume();     // b
  SliderHandle* sfx_toggle();     // c
  SliderHandle* sfx_volume();     // d
  SliderHandle* team_outline();   // e
  SliderHandle* render_rate();    // f
  SliderHandle* main_volume();    // g
  SliderHandle* ups();            // h
  ArrowSwitcher* lang_row();      // i
  SliderHandle* stats_visible();  // j

  void on_start() override;
  void on_resume() override;
  void on_stop() override;

 private:
  SliderHandle* row_slider(const std::u16string& row_id);
  ArrowSwitcher* row_switcher(const std::u16string& row_id);

  // TS `cbs: ISoundsCallback`（对象同一性 ⇒ 端口用成员 + 回指）。
  struct SoundsListener : LfwSoundsCallback {
    MiscSettingsLogic* self = nullptr;
    void on_volume_changed(double volume, double prev) override;
    void on_bgm_volume_changed(double volume, double prev) override;
    void on_sound_volume_changed(double volume, double prev) override;
    void on_bgm_muted_changed(bool muted) override;
    void on_sound_muted_changed(bool muted) override;
  } _sounds_listener;
  bool _sounds_added = false;

  std::function<void()> _world_remover;
  std::function<void()> _lfw_remover;

  SliderHandle* _a = nullptr;
  SliderHandle* _b = nullptr;
  SliderHandle* _c = nullptr;
  SliderHandle* _d = nullptr;
  SliderHandle* _e = nullptr;
  SliderHandle* _f = nullptr;
  SliderHandle* _g = nullptr;
  SliderHandle* _h = nullptr;
  ArrowSwitcher* _i = nullptr;
  SliderHandle* _j = nullptr;
};

}
}
