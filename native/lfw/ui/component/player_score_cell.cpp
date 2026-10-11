#include "lfw/ui/component/player_score_cell.h"

#include <string>

#include "lfw/entity/summary.h"
#include "lfw/entity/summary_mgr.h"
#include "lfw/ui/component/player_score.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"

namespace lfw {
namespace ui {

const ClazzTag* PlayerScoreCell::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerScoreCell::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerScoreCell"};
  return tags;
}

std::u16string PlayerScoreCell::kind() { return str(0).value_or(std::u16string()); }

PlayerScore* PlayerScoreCell::player_score() {
  UIComponent* const c = node.lookup_component(PlayerScore::class_tag());
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), PlayerScore::class_tag()) ? static_cast<PlayerScore*>(c) : nullptr;
}

void PlayerScoreCell::update(double dt) {
  (void)dt;
  Style& style = get_style();
  const std::u16string txt = get_txt();
  if (_last_txt.has_value() && *_last_txt == txt && style.version() == _last_style_version) {
    return;
  }
  _last_txt = txt;
  _last_style_version = style.version();
  node.set_text(txt);
}

Style& PlayerScoreCell::get_style() {
  PlayerScore* const s = player_score();
  Entity* const c = s != nullptr ? s->fighter() : nullptr;
  if (s == nullptr || c == nullptr) return node.style;
  if (kind() == u"status") {
    Style& style = node.style;
    if (c->hp() > 0) style.set_fill_style(node.get_value(u"win_alive_color"));
    else if (s->lose().value_or(false)) style.set_fill_style(node.get_value(u"lose_color"));
    else style.set_fill_style(node.get_value(u"win_dead_color"));
    return style;
  }
  return node.style;
}

std::u16string PlayerScoreCell::get_txt() {
  PlayerScore* const s = player_score();
  Entity* const c = s != nullptr ? s->fighter() : nullptr;
  if (s == nullptr || c == nullptr) return u"-";
  const std::u16string k = kind();
  if (k == u"kill") return to_string(summary_mgr().get(c->id)->kill_sum());
  if (k == u"attack") return to_string(summary_mgr().get(c->id)->damage_sum());
  if (k == u"hp_lost") return to_string(summary_mgr().get(c->id)->hp_lost());
  if (k == u"mp_usage") return to_string(summary_mgr().get(c->id)->mp_usage());
  if (k == u"picking") return to_string(summary_mgr().get(c->id)->picking_sum());
  if (k == u"status") {
    if (c->hp() > 0) return to_string(node.get_value(u"win_alive_txt"));
    if (s->lose().value_or(false)) return to_string(node.get_value(u"lose_txt"));
    return to_string(node.get_value(u"win_dead_txt"));
  }
  return u"-";
}

}
}
