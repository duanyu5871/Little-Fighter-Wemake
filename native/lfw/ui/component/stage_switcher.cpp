#include "lfw/ui/component/stage_switcher.h"

#include <cmath>
#include <memory>

#include "lfw/core/same_ref.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/stage_group.h"
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

bool group_has(const Value& v, const char16_t* name) {
  for (const std::u16string& g : strs_of(field_of(v, u"group"))) {
    if (g == name) return true;
  }
  return false;
}

const Value* void_stage() { return defines::find(u"Defines.VOID_STAGE"); }

std::u16string text_or_empty(const Value& obj, const char16_t* key) {
  const Value* const v = field_of(obj, key);
  if (v == nullptr) return std::u16string();
  if (const std::u16string* const s = std::get_if<std::u16string>(v)) return *s;
  return std::u16string();
}

bool truthy_value(const Value* const v) { return v != nullptr && truthy(*v); }

}

const ClazzTag* StageSwitcher::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& StageSwitcher::TAGS() {
  static const std::vector<std::u16string> tags{u"StageSwitcher"};
  return tags;
}

bool StageSwitcher::show_all() { return lfw().is_cheat(u"GIM_INK"); }

Value StageSwitcher::default_stage() {
  const Value* const v = void_stage();
  return v != nullptr ? *v : Value();
}

std::vector<Value> StageSwitcher::stages() {
  const bool cheat_0 = lfw().is_cheat(u"LF2_NET");
  const bool cheat_1 = lfw().is_cheat(u"GIM_INK");
  const std::vector<Value>& all = lfw().datas().stages();
  if (cheat_0 && cheat_1) return all;
  std::vector<Value> ret;
  for (const Value& v : all) {
    if (!cheat_0 && group_has(v, stage_group::kHidden)) continue;
    if (!cheat_1 && group_has(v, stage_group::kDev)) continue;
    if (!cheat_1 && !truthy_value(field_of(v, u"is_starting"))) continue;
    ret.push_back(v);
  }
  return ret.empty() ? all : ret;
}

std::u16string StageSwitcher::stage_text() {
  if (show_all()) return text_or_empty(_stage, u"name");
  // `this._stage.starting_name ?? this._stage.name ?? ""`
  const std::u16string starting = text_or_empty(_stage, u"starting_name");
  if (!starting.empty()) return starting;
  return text_or_empty(_stage, u"name");
}

void StageSwitcher::on_resume() {
  if (_remover) return;
  _remover = lfw().callbacks.on(u"on_broadcast",
                                [this](const LfwCallbacks::Payloads& p) {
                                  on_broadcast(p.empty() ? std::u16string() : p[0].text);
                                });
}

void StageSwitcher::on_pause() {
  if (!_remover) return;
  _remover();
  _remover = nullptr;
}

void StageSwitcher::on_show() {
  const Value* const vs = void_stage();
  if (vs != nullptr && same_ref(_stage, *vs)) on_broadcast(u"switch_stage");
}

void StageSwitcher::on_broadcast(const std::u16string& v) {
  double dir = 1.0;
  if (v == built_in_broadcast::kSwitchStage) dir = 1.0;
  else if (v == built_in_broadcast::kSwitchStageR) dir = -1.0;
  else return;
  const std::vector<Value> list = stages();
  Stage* const st = world().stage();
  if (st == nullptr) return;
  if (list.empty()) {
    if (const Value* const vs = void_stage()) _stage = *vs;
    if (const Value* const vb = defines::find(u"Defines.VOID_BG")) st->change_bg(*vb);
  } else {
    const std::u16string state_id = text_or_empty(_stage, u"id");
    double curr_idx = -1.0;
    for (size_t i = 0; i < list.size(); ++i) {
      if (text_or_empty(list[i], u"id") == state_id) {
        curr_idx = static_cast<double>(i);
        break;
      }
    }
    const double len = static_cast<double>(list.size());
    const double next_idx = std::fmod(curr_idx + len + dir, len);
    _stage = list[static_cast<size_t>(next_idx)];
    const std::u16string bg_id = text_or_empty(_stage, u"bg");
    Value bdt;
    bool found = false;
    for (const Value& bg : lfw().datas().backgrounds()) {
      if (text_or_empty(bg, u"id") != bg_id) continue;
      bdt = bg;
      found = true;
      break;
    }
    if (const Value* const vb = defines::find(u"Defines.VOID_BG")) {
      st->change_bg(found ? bdt : *vb);
    }
  }
  node.set_text(stage_text());
}

}
}
