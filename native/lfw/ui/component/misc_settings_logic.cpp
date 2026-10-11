#include "lfw/ui/component/misc_settings_logic.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/defines/sync_render_enum.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/arrow_switcher.h"
#include "lfw/ui/component/slider_handle.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace ui {

namespace {

// TS `render_rate_options`（`sync_render` 可选值的**下拉顺序**，不是枚举值顺序）。
const std::vector<double> kRenderRateOptions = {
    static_cast<double>(SyncRenderEnum::Half),   static_cast<double>(SyncRenderEnum::Sync),
    static_cast<double>(SyncRenderEnum::FPS_60), static_cast<double>(SyncRenderEnum::FPS_120),
    static_cast<double>(SyncRenderEnum::Unlimited)};

// 语言行选项顺序，须与 language_row 的 items 一致（'' 表示英文 base）。
const std::vector<std::u16string> kLangCodes = {u"",      u"zh-hans", u"zh-hant", u"de",
                                                u"es",    u"fr",      u"hu",      u"it",
                                                u"ja",    u"ko",      u"nl",      u"pl",
                                                u"pt",    u"ro",      u"ru"};

// JS `arr.indexOf(v)` 的数字口径（严格相等；NaN/null/缺失都不命中 ⇒ -1）。
double index_of_number(const std::vector<double>& arr, const Value& v) {
  if (std::holds_alternative<NullTag>(v)) return -1.0;
  const double n = to_number(v);
  if (std::isnan(n)) return -1.0;
  for (size_t i = 0; i < arr.size(); ++i) {
    if (arr[i] == n) return static_cast<double>(i);
  }
  return -1.0;
}

// JS `arr[v]`（非整数/越界 ⇒ undefined）。
Value indexed(const std::vector<double>& arr, double i) {
  if (i != std::floor(i) || i < 0 || i >= static_cast<double>(arr.size())) return Value();
  return Value(arr[static_cast<size_t>(i)]);
}

Value indexed_str(const std::vector<std::u16string>& arr, double i) {
  if (i != std::floor(i) || i < 0 || i >= static_cast<double>(arr.size())) return Value();
  return Value(arr[static_cast<size_t>(i)]);
}

// JS `arr.indexOf(str)`（非串 ⇒ -1）。
double index_of_str(const std::vector<std::u16string>& arr, const Value& v) {
  if (!std::holds_alternative<std::u16string>(v)) return -1.0;
  const std::u16string& s = std::get<std::u16string>(v);
  for (size_t i = 0; i < arr.size(); ++i) {
    if (arr[i] == s) return static_cast<double>(i);
  }
  return -1.0;
}

void play_preset_cancel(LFW& lfw) {
  lfw.host().sounds_play_preset(Value(u"cancel"), Value(), Value(), Value());
}

void play_preset_ok(LFW& lfw) {
  lfw.host().sounds_play_preset(Value(u"ok"), Value(), Value(), Value());
}

}

const ClazzTag* MiscSettingsLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& MiscSettingsLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"MiscSettingsLogic"};
  return tags;
}

SliderHandle* MiscSettingsLogic::row_slider(const std::u16string& row_id) {
  UINode* const row = node.search_node(row_id);
  if (row == nullptr) return nullptr;
  UIComponent* const c = row->search_component(SliderHandle::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), SliderHandle::class_tag()) ? static_cast<SliderHandle*>(c) : nullptr;
}

ArrowSwitcher* MiscSettingsLogic::row_switcher(const std::u16string& row_id) {
  UINode* const row = node.search_node(row_id);
  if (row == nullptr) return nullptr;
  UIComponent* const c = row->search_component(ArrowSwitcher::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), ArrowSwitcher::class_tag()) ? static_cast<ArrowSwitcher*>(c)
                                                          : nullptr;
}

SliderHandle* MiscSettingsLogic::bgm_toggle() {
  if (_a == nullptr) _a = row_slider(u"bgm_toggle_row");
  return _a;
}

SliderHandle* MiscSettingsLogic::bgm_volume() {
  if (_b == nullptr) _b = row_slider(u"bgm_volume_row");
  return _b;
}

SliderHandle* MiscSettingsLogic::sfx_toggle() {
  if (_c == nullptr) _c = row_slider(u"sfx_toggle_row");
  return _c;
}

SliderHandle* MiscSettingsLogic::sfx_volume() {
  if (_d == nullptr) _d = row_slider(u"sfx_volume_row");
  return _d;
}

SliderHandle* MiscSettingsLogic::team_outline() {
  if (_e == nullptr) _e = row_slider(u"team_outline_row");
  return _e;
}

SliderHandle* MiscSettingsLogic::render_rate() {
  if (_f == nullptr) _f = row_slider(u"render_rate_row");
  return _f;
}

SliderHandle* MiscSettingsLogic::main_volume() {
  if (_g == nullptr) _g = row_slider(u"main_volume_row");
  return _g;
}

SliderHandle* MiscSettingsLogic::ups() {
  if (_h == nullptr) _h = row_slider(u"ups_row");
  return _h;
}

ArrowSwitcher* MiscSettingsLogic::lang_row() {
  if (_i == nullptr) _i = row_switcher(u"language_row");
  return _i;
}

SliderHandle* MiscSettingsLogic::stats_visible() {
  if (_j == nullptr) _j = row_slider(u"stats_visible_row");
  return _j;
}

void MiscSettingsLogic::SoundsListener::on_volume_changed(double volume, double prev) {
  (void)prev;
  SliderHandle* const p = self != nullptr ? self->main_volume() : nullptr;
  if (p != nullptr) p->set_factor(volume);
}

void MiscSettingsLogic::SoundsListener::on_bgm_volume_changed(double volume, double prev) {
  (void)prev;
  SliderHandle* const p = self != nullptr ? self->bgm_volume() : nullptr;
  if (p != nullptr) p->set_factor(volume);
}

void MiscSettingsLogic::SoundsListener::on_sound_volume_changed(double volume, double prev) {
  (void)prev;
  SliderHandle* const p = self != nullptr ? self->sfx_volume() : nullptr;
  if (p != nullptr) p->set_factor(volume);
}

void MiscSettingsLogic::SoundsListener::on_bgm_muted_changed(bool muted) {
  SliderHandle* const p = self != nullptr ? self->bgm_toggle() : nullptr;
  if (p != nullptr) p->set_value(muted ? 0.0 : 1.0);
}

void MiscSettingsLogic::SoundsListener::on_sound_muted_changed(bool muted) {
  SliderHandle* const p = self != nullptr ? self->sfx_toggle() : nullptr;
  if (p != nullptr) p->set_value(muted ? 0.0 : 1.0);
}

void MiscSettingsLogic::on_start() {
  UIComponent::on_start();
  _sounds_listener.self = this;
  // TS `sounds.callbacks.add(cbs)` / `world.callbacks.add(...)` 都靠对象同一性去重 ⇒ 重复 on_start 不能重复订阅。
  if (!_sounds_added) {
    lfw().sounds_callback_add(&_sounds_listener);
    _sounds_added = true;
  }
  if (!_world_remover) {
    _world_remover = world().callbacks.on(
        u"on_dataset_change", [this](const WorldCallbacks::Payloads& ps) {
          for (const WorldCallbackArgs& a : ps) {
            if (a.key != u"sync_render") continue;
            SliderHandle* const p = render_rate();
            if (p != nullptr) p->set_value(index_of_number(kRenderRateOptions, a.value));
          }
        });
  }
  if (!_lfw_remover) {
    _lfw_remover = lfw().callbacks.on(u"on_broadcast", [this](const LfwCallbacks::Payloads& ps) {
      for (const LfwCallbackArgs& a : ps) {
        SliderHandle* const p = stats_visible();
        if (p == nullptr) continue;
        if (a.text == u"stats_visible:1") p->set_value(1.0);
        else if (a.text == u"stats_visible:0") p->set_value(0.0);
      }
    });
  }
  SliderHandle* const mv = main_volume();
  if (mv != nullptr) {
    mv->on_value_changed([this](double, SliderHandle& s) {
      lfw().sounds_set_volume(s.factor());
      play_preset_cancel(lfw());
    });
  }
  SliderHandle* const bt = bgm_toggle();
  if (bt != nullptr) {
    bt->on_value_changed([this](double v, SliderHandle&) {
      lfw().sounds_set_bgm_muted(!truthy(Value(v)));
      if (!truthy(Value(v))) play_preset_cancel(lfw());
      else play_preset_ok(lfw());
    });
  }
  SliderHandle* const bv = bgm_volume();
  if (bv != nullptr) {
    bv->on_value_changed([this](double, SliderHandle& s) {
      lfw().sounds_set_bgm_volume(s.factor());
      play_preset_cancel(lfw());
    });
  }
  SliderHandle* const st = sfx_toggle();
  if (st != nullptr) {
    st->on_value_changed([this](double v, SliderHandle&) {
      lfw().sounds_set_sound_muted(!truthy(Value(v)));
      if (!truthy(Value(v))) play_preset_cancel(lfw());
      else play_preset_ok(lfw());
    });
  }
  SliderHandle* const sv = sfx_volume();
  if (sv != nullptr) {
    sv->on_value_changed([this](double, SliderHandle& s) {
      lfw().sounds_set_sound_volume(s.factor());
      play_preset_cancel(lfw());
    });
  }
  SliderHandle* const to = team_outline();
  if (to != nullptr) {
    to->on_value_changed([this](double v, SliderHandle&) {
      world().dataset.set(u"outline_enabled", Value(v));
      if (truthy(Value(v))) play_preset_ok(lfw());
      else play_preset_cancel(lfw());
    });
  }
  SliderHandle* const rr = render_rate();
  if (rr != nullptr) {
    rr->on_value_changed([this](double v, SliderHandle&) {
      world().dataset.set(u"sync_render", indexed(kRenderRateOptions, v));
      play_preset_ok(lfw());
    });
  }
  const std::vector<double> ups_arr = {30.0, 60.0};
  std::vector<double> atom_time_arr;
  std::vector<double> double_click_interval_arr;
  std::vector<double> key_hit_duration_arr;
  for (const double v : ups_arr) {
    atom_time_arr.push_back(round_float(60.0 / v));
    double_click_interval_arr.push_back(30.0 * v / 60.0);
    key_hit_duration_arr.push_back(10.0 * v / 60.0);
  }
  SliderHandle* const up = ups();
  if (up != nullptr) {
    up->on_value_changed([this, ups_arr, atom_time_arr, double_click_interval_arr,
                          key_hit_duration_arr](double v, SliderHandle&) {
      world().dataset.set(u"UPS", indexed(ups_arr, v));
      world().dataset.set(u"atom_time", indexed(atom_time_arr, v));
      world().dataset.set(u"wait_offset", Value(0.0));
      world().dataset.set(u"fvy_f", Value(-0.5));
      world().dataset.set(u"double_click_interval", indexed(double_click_interval_arr, v));
      world().dataset.set(u"key_hit_duration", indexed(key_hit_duration_arr, v));
      play_preset_ok(lfw());
    });
  }
  ArrowSwitcher* const lr = lang_row();
  if (lr != nullptr) {
    lr->on_value_changed([this](double v, ArrowSwitcher&) {
      lfw().set_lang(indexed_str(kLangCodes, v));
      play_preset_ok(lfw());
    });
  }
  SliderHandle* const svis = stats_visible();
  if (svis != nullptr) {
    svis->on_value_changed([this](double v, SliderHandle&) {
      std::u16string msg = u"stats_visible_set:";
      msg += truthy(Value(v)) ? u"1" : u"0";
      lfw().broadcast(Value(msg));
      if (truthy(Value(v))) play_preset_ok(lfw());
      else play_preset_cancel(lfw());
    });
  }
}

void MiscSettingsLogic::on_resume() {
  UIComponent::on_resume();
  lfw().broadcast(Value(u"stats_visible_get"));
  SliderHandle* const mv = main_volume();
  if (mv != nullptr) mv->set_factor(lfw().sounds_volume());
  SliderHandle* const bt = bgm_toggle();
  if (bt != nullptr) bt->set_value(lfw().sounds_bgm_muted() ? 0.0 : 1.0);
  SliderHandle* const bv = bgm_volume();
  if (bv != nullptr) bv->set_factor(lfw().sounds_bgm_volume());
  SliderHandle* const sv = sfx_volume();
  if (sv != nullptr) sv->set_factor(lfw().sounds_sound_volume());
  SliderHandle* const st = sfx_toggle();
  if (st != nullptr) st->set_value(lfw().sounds_sound_muted() ? 0.0 : 1.0);
  SliderHandle* const to = team_outline();
  if (to != nullptr) to->set_factor(to_number(world().dataset.get(u"outline_enabled")));
  SliderHandle* const rr = render_rate();
  if (rr != nullptr) rr->set_value(index_of_number(kRenderRateOptions, world().dataset.get(u"sync_render")));
  const std::vector<double> ups_arr = {30.0, 60.0};
  const double ups_idx = index_of_number(ups_arr, world().dataset.get(u"UPS"));
  SliderHandle* const up = ups();
  if (up != nullptr) up->set_value(ups_idx < 0 ? 1.0 : ups_idx);
  const Value cur_lang = lfw().canonical_lang();
  ArrowSwitcher* const lr = lang_row();
  if (lr != nullptr) lr->set_value(index_of_str(kLangCodes, cur_lang));
}

void MiscSettingsLogic::on_stop() {
  UIComponent::on_stop();
  if (_sounds_added) {
    lfw().sounds_callback_del(&_sounds_listener);
    _sounds_added = false;
  }
  if (_world_remover) {
    _world_remover();
    _world_remover = nullptr;
  }
  if (_lfw_remover) {
    _lfw_remover();
    _lfw_remover = nullptr;
  }
}

}
}
