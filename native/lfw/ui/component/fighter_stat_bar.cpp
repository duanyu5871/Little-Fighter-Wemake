#include "lfw/ui/component/fighter_stat_bar.h"

#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/team_color.h"
#include "lfw/controller/base_controller.h"
#include "lfw/defines/team_enum.h"
#include "lfw/lfw.h"
#include "lfw/player_info.h"
#include "lfw/ui/component/picture.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/is_class.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/string_help.h"

namespace lfw {
namespace ui {

namespace {

Value base_of(const Value& data) { return field_or(data, u"base"); }

std::u16string str_field(const Value& v, const char16_t* key) {
  const Value* const f = field_of(v, key);
  if (f == nullptr) return std::u16string();
  const std::u16string* const s = std::get_if<std::u16string>(f);
  return s != nullptr ? *s : std::u16string();
}

std::optional<std::u16string> str_opt(const Value& v) {
  const std::u16string* const s = std::get_if<std::u16string>(&v);
  if (s != nullptr) return std::optional<std::u16string>(*s);
  return std::optional<std::u16string>();
}

bool field_eq_str(const Value& v, const std::u16string& s) {
  const std::u16string* const cur = std::get_if<std::u16string>(&v);
  return cur != nullptr && *cur == s;
}

}

const ClazzTag* FighterStatBar::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& FighterStatBar::TAGS() {
  static const std::vector<std::u16string> tags{u"FighterStatBar"};
  return tags;
}

const Value& FighterStatBar::PROPS() {
  static const Value meta = [] {
    auto node_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto picture_prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:Picture"));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"dark_hp_bar", Value(node_prop()));
    o->set(u"hp_bar", Value(node_prop()));
    o->set(u"dark_mp_bar", Value(node_prop()));
    o->set(u"mp_bar", Value(node_prop()));
    o->set(u"fall_value_bar", Value(node_prop()));
    o->set(u"defend_value_bar", Value(node_prop()));
    o->set(u"toughness_bar", Value(node_prop()));
    o->set(u"head_img", Value(picture_prop()));
    o->set(u"name_txt", Value(node_prop()));
    return Value(o);
  }();
  return meta;
}

const std::u16string& FighterStatBar::props_tag() const {
  static const std::u16string tag = u"FighterStatBar";
  return tag;
}

FighterStatBar::FighterStatBar(UINode& layout, const std::u16string& f_name, const Value& info)
    : UIComponent(layout, f_name, info) {
  defend_value_max.handler([this](SmoothNumber&) { update_defend_value(); });
  defend_value.handler([this](SmoothNumber&) { update_defend_value(); });
  fall_value_max.handler([this](SmoothNumber&) { update_fall_value(); });
  fall_value.handler([this](SmoothNumber&) { update_fall_value(); });
  toughness.handler([this](SmoothNumber&) { update_toughness(); });
  toughness_max.handler([this](SmoothNumber&) { update_toughness(); });
  hp_max.handler([this](SmoothNumber&) {
    update_hp();
    update_hp_r();
  });
  hp_r.handler([this](SmoothNumber&) { update_hp_r(); });
  hp.handler([this](SmoothNumber&) { update_hp(); });
  mp_max.handler([this](SmoothNumber&) {
    update_mp_max();
    update_mp();
  });
  mp.handler([this](SmoothNumber&) { update_mp(); });
}

UINode* FighterStatBar::prop_node(const std::u16string& prop_name) {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(prop_name));
}

Picture* FighterStatBar::head_img() {
  if (!ensure_props()) return nullptr;
  UIComponent* const c = ref_to_comp(props_holder.prop_instance(u"head_img"));
  if (c == nullptr) return nullptr;
  return is_class(c->clazz(), Picture::class_tag()) ? static_cast<Picture*>(c) : nullptr;
}

void FighterStatBar::set_entity(Entity* e) {
  const std::optional<std::u16string> new_id =
      e != nullptr ? std::optional<std::u16string>(e->id) : std::optional<std::u16string>();
  if (entity == e && _eid == new_id) return;
  _eid = entity != nullptr ? std::optional<std::u16string>(entity->id)
                           : std::optional<std::u16string>();
  entity = e;
}

void FighterStatBar::on_show() {
  direction = props_holder.str(u"direction").value_or(std::u16string());
}

void FighterStatBar::update_hp_tier_colors(const std::vector<UINode*>& tiers, bool flashing) {
  if (flashing == hp_flashing) return;
  hp_flashing = flashing;
  if (!_tier_colors.has_value()) {
    std::vector<std::u16string> colors;
    colors.reserve(tiers.size());
    for (const UINode* const t : tiers) colors.push_back(t->color);
    _tier_colors = std::move(colors);
  }
  std::vector<std::u16string>& colors = *_tier_colors;
  for (int i = static_cast<int>(tiers.size()) - 1; i >= 0; --i) {
    UINode* const tier = tiers[static_cast<size_t>(i)];
    if (!tier->visible()) tier->color = colors[static_cast<size_t>(i)];
    else tier->color = flashing ? u"rgb(255,130,130)" : colors[static_cast<size_t>(i)];
  }
}

void FighterStatBar::update_defend_value(std::optional<double> val, std::optional<double> max) {
  update_bar(prop_node(u"defend_value_bar"), val.value_or(defend_value.value()),
             max.value_or(defend_value_max.value()));
}

void FighterStatBar::update_fall_value(std::optional<double> val, std::optional<double> max) {
  update_bar(prop_node(u"fall_value_bar"), val.value_or(fall_value.value()),
             max.value_or(fall_value_max.value()));
}

void FighterStatBar::update_toughness(std::optional<double> val, std::optional<double> max) {
  update_bar(prop_node(u"toughness_bar"), val.value_or(toughness.value()),
             max.value_or(toughness_max.value()));
}

void FighterStatBar::update_hp(std::optional<double> val) {
  update_tiers(prop_node(u"hp_bar"), val.value_or(hp.value()));
}

void FighterStatBar::update_hp_r(std::optional<double> val) {
  update_tiers(prop_node(u"dark_hp_bar"), val.value_or(hp_r.value()));
}

void FighterStatBar::update_tiers(UINode* bar, double val) {
  if (bar == nullptr) return;
  const std::vector<UINode*>& tiers = bar->children();
  if (tiers.size() < 2) return;
  for (size_t i = 0; i < tiers.size(); i++) {
    UINode* const tier = tiers[i];
    const double vis = clamp(val - static_cast<double>(i) * 500.0, 0.0, 500.0);
    const bool visible = !dead;
    if (tier->visible() != visible) tier->set_visible(visible);
    update_bar(tier, vis, 500.0);
  }
}

void FighterStatBar::update_bar(UINode* bar, double val, double max) {
  if (bar == nullptr) return;
  if (max == 0) bar->set_scale(0.0);
  else bar->set_scale(val / max);
}

void FighterStatBar::update_mp(std::optional<double> val, std::optional<double> max) {
  update_bar(prop_node(u"mp_bar"), val.value_or(mp.value()), max.value_or(mp_max.value()));
}

void FighterStatBar::update_mp_max() {
  UINode* const dark_bar = prop_node(u"dark_mp_bar");
  if (dark_bar == nullptr) return;
  dark_bar->set_scale(dead ? 0.0 : 1.0, 1.0, 1.0);
}

void FighterStatBar::update_head() {
  const std::u16string head =
      entity != nullptr ? str_field(base_of(entity->data()), u"head") : std::u16string();
  if (_head.has_value() && *_head == head) return;
  Picture* const img = head_img();
  if (img != nullptr) img->set_src(head);
  _head = head;
}

void FighterStatBar::update_name() {
  UINode* const name_txt = prop_node(u"name_txt");
  if (name_txt == nullptr) return;
  Entity* const ent = entity;
  const controller::BaseController* const ctrl = ent != nullptr ? ent->ctrl() : nullptr;
  const PlayerInfo* const player =
      ctrl != nullptr ? lfw().players_get(Value(std::u16string(ctrl->player_id))) : nullptr;
  const bool taken_over = ent != nullptr && ctrl != nullptr && ctrl->is_bot() &&
                          player != nullptr && !player->is_com();
  std::u16string name0 =
      ent != nullptr ? js_trim(to_string(ent->name())) : std::u16string(u"NONE");
  if (taken_over) name0 += u"(bot)";
  const std::u16string name1 =
      ent != nullptr ? js_trim(str_field(base_of(ent->data()), u"name")) : std::u16string();
  std::u16string name_s = !name0.empty() ? name0 : name1;
  if (name0 != name1 && !name0.empty() && !name1.empty()) name_s = name1 + u" [" + name0 + u"]";
  const double version = name_txt->style.version();
  const std::optional<std::u16string> outline = str_opt(name_txt->outlineColor());
  if (_name.has_value() && *_name == name_s && _name_version == version &&
      _name_outline == outline) {
    return;
  }
  _name = name_s;
  _name_version = version;
  _name_outline = outline;
  name_txt->set_text(name_s);
}

void FighterStatBar::update_team() {
  UINode* const name_txt = prop_node(u"name_txt");
  if (name_txt == nullptr) return;
  const std::u16string team = entity != nullptr ? entity->team() : std::u16string();
  const Object* team_info_obj = team_color_detail::team_info(team);
  if (team_info_obj == nullptr) {
    team_info_obj = team_color_detail::team_info(std::u16string(team_enum::kIndependent));
  }
  const std::u16string* const toc =
      team_color_detail::team_field(team_info_obj, u"txt_outline_color");
  const std::u16string* const tc = team_color_detail::team_field(team_info_obj, u"txt_color");
  const std::u16string outline_color = toc != nullptr ? *toc : std::u16string();
  const std::u16string txt_color = tc != nullptr ? *tc : std::u16string();
  if (field_eq_str(name_txt->outlineColor(), outline_color) &&
      field_eq_str(name_txt->style.fill_style(), txt_color)) {
    return;
  }
  name_txt->set_outline_color(Value(outline_color));
  name_txt->style.set_fill_style(Value(txt_color));
  name_txt->style.touch();
}

void FighterStatBar::update(double dt) {
  (void)dt;
  update_team();
  update_name();
  update_head();
  defend_value.update();
  defend_value_max.update();
  fall_value.update();
  fall_value_max.update();
  toughness.update();
  toughness_max.update();
  mp.update();
  mp_max.update();
  hp.update();
  hp_r.update();
  hp_max.update();

  Entity* const ent = entity;
  UINode* const hp_bar = prop_node(u"hp_bar");
  UINode* const mp_bar = prop_node(u"mp_bar");
  if (ent == nullptr) return;

  if (hp_bar != nullptr) {
    const bool flashing =
        ent->marks.count(u"Healing") > 0 && std::fmod(ent->lifetime(), 8.0) < 4.0;
    update_hp_tier_colors(hp_bar->children(), flashing);
  }
  if (mp_bar != nullptr) {
    mp_healing = ent->marks.count(u"MpHealing") > 0 && std::fmod(ent->lifetime(), 8.0) < 4.0;
    if (!mp_bar->children().empty()) {
      mp_bar->children()[0]->color = mp_healing ? u"rgb(130,130,255)" : u"rgb(0,0,255)";
    }
  }
  dead = ent->hp() <= 0;
  defend_value.set_target(dead ? 0.0 : ent->defend_value());
  defend_value_max.set_target(dead ? 1.0 : ent->defend_value_max());
  fall_value.set_target(dead ? 0.0 : ent->fall_value());
  fall_value_max.set_target(dead ? 1.0 : ent->fall_value_max());
  toughness.set_target(dead ? 0.0 : ent->toughness());
  toughness_max.set_target(dead ? 1.0 : ent->toughness_max());
  mp.set_target(dead ? 0.0 : ent->mp());
  mp_max.set_target(dead ? 1.0 : ent->mp_max());
  hp.set_target(dead ? 0.0 : ent->hp());
  hp_r.set_target(dead ? 0.0 : ent->hp_r());
  hp_max.set_target(dead ? 1.0 : ent->hp_max());
}

}
}
