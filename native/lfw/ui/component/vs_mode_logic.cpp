#include "lfw/ui/component/vs_mode_logic.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/defines/game_key.h"
#include "lfw/lfw.h"
#include "lfw/ui/component/fighter_stat_bar.h"
#include "lfw/ui/component/mode_state.h"
#include "lfw/ui/ui_event.h"
#include "lfw/ui/uilayer.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"

namespace lfw {
namespace ui {

namespace {

// JS `/p(\d)_stat/` 的首个匹配（捕获一位数字）；无匹配 ⇒ undefined（`Number(undefined)` = NaN，
// 与任何数比较都为 false）。
std::optional<double> stat_index_of(const std::u16string& id) {
  for (size_t i = 0; i + 2 <= id.size(); ++i) {
    if (id[i] != u'p') continue;
    size_t j = i + 1;
    const size_t d0 = j;
    while (j < id.size() && id[j] >= u'0' && id[j] <= u'9') ++j;
    if (j - d0 != 1) continue;
    if (j + 5 > id.size()) continue;
    if (id.compare(j, 5, u"_stat") != 0) continue;
    return to_number(Value(id.substr(d0, 1)));
  }
  return std::nullopt;
}

}

const ClazzTag* VsModeLogic::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& VsModeLogic::TAGS() {
  static const std::vector<std::u16string> tags{u"VsModeLogic"};
  return tags;
}

VsModeLogic::VsModeLogic(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {
  _states.push_back(std::make_unique<GameModeFSMState_Running>(*this));
  _states.push_back(std::make_unique<GameModeFSMState_BeforeEnd>(*this));
  _states.push_back(std::make_unique<GameModeFSMState_End>(*this));
  _fsm.add(_states[0].get(), _states[1].get(), _states[2].get());
}

void VsModeLogic::on_start() {
  UIComponent::on_start();
  _fsm.use(Value(std::u16string(mode_state_str(ModeState::Running))));
  world().set_paused(false);
  world().dataset.set(u"playrate", Value(1.0));
  world().dataset.set(u"infinity_mp", Value(0.0));

  std::vector<FighterStatBar*> stat_bars;
  for (UIComponent* const c : node.search_components(FighterStatBar::class_tag())) {
    if (c != nullptr && is_class(c->clazz(), FighterStatBar::class_tag())) {
      stat_bars.push_back(static_cast<FighterStatBar*>(c));
    }
  }

  double player_count = 0;
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.second->fighter() != nullptr) player_count += 1;
  }
  for (size_t i = 0; i < stat_bars.size();) {
    FighterStatBar* const stat_bar = stat_bars[i];
    // TS 的 `node.id?.startsWith(...)`：id 缺失 ⇒ undefined。非字符串 ⇒ 空串（等价不可达匹配）。
    const Value id_value = stat_bar->node.id();
    const std::u16string id =
        std::holds_alternative<std::u16string>(id_value) ? std::get<std::u16string>(id_value)
                                                         : std::u16string();
    bool enabled = false;
    if (player_count == 2) {  // 1 on 1.
      enabled = id.starts_with(u"_1v1_fighter_stat_");
    } else {
      const std::optional<double> n = stat_index_of(id);
      enabled = n.has_value() && player_count >= *n;
    }
    stat_bar->node.set_visible(enabled);
    stat_bar->node.set_disabled(!enabled);
    if (enabled) {
      ++i;
      continue;
    }
    stat_bars.erase(stat_bars.begin() + static_cast<std::ptrdiff_t>(i));
  }

  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.second->fighter() == nullptr) continue;
    if (stat_bars.empty()) break;
    FighterStatBar* const stat_bar = stat_bars.front();
    stat_bars.erase(stat_bars.begin());
    stat_bar->set_entity(kv.second->fighter());
  }
}

void VsModeLogic::on_stop() {
  UIComponent::on_stop();
  world().clear();
}

void VsModeLogic::update(double dt) {
  _fsm.update(dt);
}

void VsModeLogic::on_key_down(LFWKeyEvent& e) {
  if (e.game_key == gk::ka || e.game_key == gk::kj) {
    const bool end_state =
        _fsm.state() != nullptr && strict_equals(_fsm.state()->key(), Value(u"End"));
    if (end_state && _fsm.state_time() > 1000) {
      e.stop_immediate_propagation();
      UIPopPageOpts opts;
      opts.min_pages = 1;
      node.pop_page(opts);
    }
  }
}

}
}
