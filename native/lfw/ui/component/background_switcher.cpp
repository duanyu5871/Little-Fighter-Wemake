#include "lfw/ui/component/background_switcher.h"

#include <cmath>
#include <memory>

#include "lfw/core/same_ref.h"
#include "lfw/defines/background_group.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/defines_data.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

std::vector<std::u16string> strs_of(const Value* const v) {
  std::vector<std::u16string> out;
  const Array* const a = v != nullptr ? as_array(*v) : nullptr;
  if (a == nullptr) return out;
  for (size_t i = 0; i < a->size(); ++i) {
    if (const std::u16string* const s = std::get_if<std::u16string>(&a->at(i))) out.push_back(*s);
  }
  return out;
}

bool base_group_has(const Value& bg, const char16_t* name) {
  const Value* const base = field_of(bg, u"base");
  if (base == nullptr) return false;
  for (const std::u16string& g : strs_of(field_of(*base, u"group"))) {
    if (g == name) return true;
  }
  return false;
}

std::u16string text_of(const Value& obj, const char16_t* key) {
  const Value* const v = field_of(obj, key);
  if (v == nullptr) return std::u16string();
  if (const std::u16string* const s = std::get_if<std::u16string>(v)) return *s;
  return std::u16string();
}

// `this._background.base.name || this._background.id`。
std::u16string bg_title(const Value& bg) {
  const Value* const base = field_of(bg, u"base");
  const std::u16string name = base != nullptr ? text_of(*base, u"name") : std::u16string();
  if (!name.empty()) return name;
  return text_of(bg, u"id");
}

const Value* define(const char16_t* key) { return defines::find(std::u16string(u"Defines.") + key); }

bool dataset_flag(World& w, const char16_t* key) { return truthy(w.dataset.get(std::u16string(key))); }

}

const ClazzTag* BackgroundSwitcher::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& BackgroundSwitcher::TAGS() {
  static const std::vector<std::u16string> tags{u"BackgroundSwitcher"};
  return tags;
}

Value BackgroundSwitcher::default_background() {
  const Value* const v = define(u"RANDOM_BG");
  return v != nullptr ? *v : Value();
}

std::vector<Value> BackgroundSwitcher::backgrounds() {
  std::vector<Value> set;
  if (const Value* const rb = define(u"RANDOM_BG")) set.push_back(*rb);
  const bool lf2_net = dataset_flag(world(), u"LF2_NET");
  const bool gim_ink = dataset_flag(world(), u"GIM_INK");
  for (const Value& bg : lfw().datas().backgrounds()) {
    if (base_group_has(bg, background_group::kRegular)) {
      // 去重按对象同一性（TS 的 `Set`）。
      bool dup = false;
      for (const Value& v : set) {
        if (same_ref(v, bg)) {
          dup = true;
          break;
        }
      }
      if (!dup) set.push_back(bg);
    } else if (base_group_has(bg, background_group::kHidden)) {
      if (lf2_net) set.push_back(bg);
    } else {
      if (gim_ink) set.push_back(bg);
    }
  }
  const Value* const vb = define(u"VOID_BG");
  const std::u16string void_id = vb != nullptr ? text_of(*vb, u"id") : std::u16string();
  std::vector<Value> ret;
  for (const Value& v : set) {
    if (text_of(v, u"id") != void_id) ret.push_back(v);
  }
  return ret;
}

void BackgroundSwitcher::on_resume() {
  node.set_text(bg_title(_background));
  if (_remover) return;
  _remover = lfw().callbacks.on(u"on_broadcast",
                                [this](const LfwCallbacks::Payloads& p) {
                                  on_broadcast(p.empty() ? std::u16string() : p[0].text);
                                });
}

void BackgroundSwitcher::on_pause() {
  if (!_remover) return;
  _remover();
  _remover = nullptr;
}

void BackgroundSwitcher::on_show() {
  const Value* const vb = define(u"VOID_BG");
  if (vb != nullptr && same_ref(_background, *vb)) on_broadcast(u"switch_background");
}

void BackgroundSwitcher::on_broadcast(const std::u16string& v) {
  double dir = 1.0;
  if (v == built_in_broadcast::kSwitchBackground) dir = 1.0;
  else if (v == built_in_broadcast::kSwitchBackgroundR) dir = -1.0;
  else if (v == built_in_broadcast::kResetBackground) dir = 0.0;
  else return;
  const std::vector<Value> list = backgrounds();
  const Value* const vb = define(u"VOID_BG");
  if (vb == nullptr) return;
  Stage* const st = world().stage();
  if (st == nullptr) return;
  if (list.empty()) {
    _background = *vb;
    st->change_bg(*vb);
  } else if (dir != 0.0) {
    const std::u16string background_id = text_of(_background, u"id");
    double curr_idx = -1.0;
    for (size_t i = 0; i < list.size(); ++i) {
      if (text_of(list[i], u"id") == background_id) {
        curr_idx = static_cast<double>(i);
        break;
      }
    }
    const double len = static_cast<double>(list.size());
    const double next_idx = std::fmod(curr_idx + len + dir, len);
    _background = list[static_cast<size_t>(next_idx)];
    st->change_bg(_background);
  } else {
    _background = list[0];
    st->change_bg(_background);
  }
  node.set_text(bg_title(_background));
}

}
}
