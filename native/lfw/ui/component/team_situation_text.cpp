#include "lfw/ui/component/team_situation_text.h"

#include <cmath>
#include <string>
#include <vector>

#include "lfw/base/team_color.h"
#include "lfw/defines/team_enum.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/component/summary_logic.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace ui {

namespace {

bool team_truthy(const std::optional<std::u16string>& v) {
  return v.has_value() && !v->empty();
}

}

const ClazzTag* TeamSituationText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& TeamSituationText::TAGS() {
  static const std::vector<std::u16string> tags{u"TeamSituationText"};
  return tags;
}

TeamSituationText::StageListener::StageListener(TeamSituationText* o) {
  handlers.push_back({u"on_stage_change", [o](const WorldPayloads& args) {
                        for (const WorldCallbackArgs& a : args) {
                          if (a.stage != nullptr) o->set_team(to_string(a.stage->team()));
                        }
                      }});
}

TeamSituationText::TeamSituationText(UINode& layout, const std::u16string& f_name,
                                     const Value& info)
    : UIComponent(layout, f_name, info) {
  _style.set_fill_style(Value(u"white"));
  _style.set_font(Value(u"12px Arial"));
  _style.set_line_width(Value(1.0));
}

void TeamSituationText::on_start() {
  _team = str(0);
  if (team_truthy(_team)) {
    set_team(*_team);
  } else {
    world().callbacks.add(&_world_cb);
    if (world().stage() != nullptr) set_team(to_string(world().stage()->team()));
  }
}

void TeamSituationText::on_stop() {
  if (!team_truthy(_team)) world().callbacks.del(&_world_cb);
}

void TeamSituationText::on_resume() { rr(); }

void TeamSituationText::update(double dt) {
  (void)dt;
  if (std::fmod(world().game_time(), 30.0) == 0.0) rr();
}

void TeamSituationText::set_team(const std::u16string& team) {
  const std::u16string fallback =
      get_team_text_color(std::u16string(team_enum::kTeam_2), nullptr);
  _style = Style();
  _style.set_fill_style(Value(get_team_text_color(team, &fallback)));
  _style.set_font(Value(u"12px Arial"));
  _style.set_line_width(Value(1.0));
  UIComponent* const c = node.root().search_component(SummaryLogic::class_tag());
  SummaryLogic* const logic =
      c != nullptr && is_class(c->clazz(), SummaryLogic::class_tag())
          ? static_cast<SummaryLogic*>(c)
          : nullptr;
  _sum_logic = logic;
  _sum_team = team;
  rr();
}

void TeamSituationText::rr() {
  if (_sum_logic == nullptr) return;
  TeamSumInfo& sum = _sum_logic->team_sum(_sum_team);
  std::u16string text = u"Man: " + number_to_string(sum.lives) + u"　HP: " +
                        number_to_string(max(0.0, floor(sum.hp)));
  if (sum.reserve != 0) text += u"　Reserve: " + number_to_string(sum.reserve);
  if (_text == text) return;
  _text = text;
  node.set_text(text, _style.data());
}

}
}
