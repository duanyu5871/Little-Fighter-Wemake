#include "lfw/ui/component/little_funny_auto_game.h"

#include <utility>

#include "lfw/defines/entity_group.h"
#include "lfw/entity/entity.h"
#include "lfw/factory.h"
#include "lfw/lfw.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

namespace {

// `v == null`（`camera.cpp` / `zip_mgr.cpp` 的既有写法）。
bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// `data.base.ce ?? 1`：不设置强度就按 1 个人算。
double ce_of(const Value& data) {
  const Value ce = field_or(field_or(data, u"base"), u"ce");
  return is_nullish(ce) ? 1.0 : to_number(ce);
}

}

const ClazzTag* LittleFunnyAutoGame::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& LittleFunnyAutoGame::TAGS() {
  static const std::vector<std::u16string> tags{u"LittleFunnyAutoGame"};
  return tags;
}

LittleFunnyAutoGame::FighterListenerT::FighterListenerT(LittleFunnyAutoGame* o) {
  handlers.push_back({u"on_dead", [o](const ValuePayloads& a) {
                        if (!a.empty()) o->on_fighter_dead(a[0]);
                      }});
}

LittleFunnyAutoGame::LittleFunnyAutoGame(UINode& layout, const std::u16string& f_name,
                                         const Value& info)
    : UIComponent(layout, f_name, info) {
  _director.offset(5000.0, {[this]() {
                               _team1.boss = add_random_fighter(0, u"1");
                               _team2.boss = add_random_fighter(1, u"2");
                               handle_soldiers();
                             }});
}

Entity* LittleFunnyAutoGame::boss(int team) const {
  return team == 1 ? _team1.boss : team == 2 ? _team2.boss : nullptr;
}

const std::vector<Entity*>& LittleFunnyAutoGame::soldiers(int team) const {
  static const std::vector<Entity*> empty;
  return team == 1 ? _team1.soldiers : team == 2 ? _team2.soldiers : empty;
}

void LittleFunnyAutoGame::on_fighter_dead(const Value& ref) {
  Entity* const e = world().find_entity(to_string(field_or(ref, u"id")));
  if (e == nullptr) return;
  e->callbacks.del(&_fighter_cbs);
  if (stopped || paused) return;
  _lr = (_lr + 1) % 2;
  const std::u16string team = e->team();
  if (team == u"1" || team == u"2") {
    Teams& t = team == u"1" ? _team1 : _team2;
    if (t.boss == e) {
      Entity* const fighter = add_random_fighter(_lr, team);
      if (fighter == nullptr) return;
      t.boss = fighter;
    } else {
      for (size_t i = 0; i < t.soldiers.size(); ++i) {
        if (t.soldiers[i] != e) continue;
        t.soldiers.erase(t.soldiers.begin() + static_cast<std::ptrdiff_t>(i));
        break;
      }
    }
    handle_soldiers();
  }
}

void LittleFunnyAutoGame::handle_soldiers() {
  // 让敌方士兵数量与我方 BOSS 的强度匹配（`ce` 高出 1 的部分 = 需要的人数差）。
  Entity* const boss1 = _team1.boss;
  Entity* const boss2 = _team2.boss;
  if (boss1 == nullptr || boss2 == nullptr) return;
  const double ce1 = ce_of(boss1->data()) - 1;
  const double ce2 = ce_of(boss2->data()) - 1;
  const std::vector<Value> group = lfw().datas().get_fighters_of_group(entity_group::k_3000);
  auto group_arr = std::make_shared<Array>();
  for (const Value& v : group) group_arr->push_back(v);
  const Value _3000 = Value(group_arr);

  std::vector<Entity*> freeguys;
  while (static_cast<double>(_team1.soldiers.size()) > ce2) {
    Entity* const freeguy = _team1.soldiers.back();
    _team1.soldiers.pop_back();
    if (freeguy == nullptr) break;
    freeguys.push_back(freeguy);
  }
  while (static_cast<double>(_team2.soldiers.size()) > ce1) {
    Entity* const freeguy = _team2.soldiers.back();
    _team2.soldiers.pop_back();
    if (freeguy == nullptr) break;
    freeguys.push_back(freeguy);
  }
  while (static_cast<double>(_team1.soldiers.size()) < ce2) {
    lfw().mt()->mark = u"lfag_3000_1";
    Entity* newguy = freeguys.empty() ? nullptr : freeguys.back();
    if (newguy == nullptr) {
      _lr = (_lr + 1) % 2;
      newguy = add_random_fighter(_lr, u"1", lfw().mt()->pick_value(_3000));
    } else {
      freeguys.pop_back();
    }
    if (newguy == nullptr) break;
    if (newguy->team() != u"1") {
      world().etc(newguy->position.x, newguy->position.y, newguy->position.z, u"6");
      newguy->set_team(u"1");
    }
    _team1.soldiers.push_back(newguy);
  }
  while (static_cast<double>(_team2.soldiers.size()) < ce1) {
    lfw().mt()->mark = u"lfag_3000_2";
    Entity* newguy = freeguys.empty() ? nullptr : freeguys.back();
    if (newguy == nullptr) {
      _lr = (_lr + 1) % 2;
      newguy = add_random_fighter(_lr, u"2", lfw().mt()->pick_value(_3000));
    } else {
      freeguys.pop_back();
    }
    if (newguy == nullptr) break;
    if (newguy->team() != u"2") {
      world().etc(newguy->position.x, newguy->position.y, newguy->position.z, u"6");
      newguy->set_team(u"2");
    }
    _team2.soldiers.push_back(newguy);
  }
  for (Entity* const guy : freeguys) guy->set_team(u"3");
}

Entity* LittleFunnyAutoGame::add_random_fighter(int lr, const std::u16string& team,
                                                const Value& data) {
  Value picked = data;
  if (!truthy(picked)) {
    Array* const arr = as_array(_datas);
    if (arr != nullptr && arr->size() == 0) {
      for (const Value& v : lfw().datas().fighters()) arr->push_back(v);
    }
    lfw().mt()->mark = u"lfag_random_fighter";
    picked = lfw().mt()->take_value(_datas);
  }
  if (!truthy(picked)) return nullptr;
  Entity* const fighter = lfw().create_entity_with_bot(std::u16string(), world(), picked);
  if (fighter == nullptr) return nullptr;
  fighter->set_team(team);
  const double facing = lr != 0 ? -1.0 : 1.0;
  fighter->facing = facing;
  _ox = (_ox + 1) % 2;
  const double x = _ox * -facing;
  fighter->callbacks.add(&_fighter_cbs);
  fighter->set_position(Value(lr * world().bg()->width() + x), Value(), Value());
  const double hp = 150.0 * ce_of(picked);
  fighter->set_hp(hp);
  fighter->set_hp_r(hp);
  fighter->set_hp_max(hp);
  fighter->stat_bar = 0;
  fighter->wakeup_invuln = 1;
  fighter->set_blinking(64);
  fighter->set_invulnerable(64);
  fighter->dead_gone = 1;
  fighter->name_visible = 0;
  fighter->enter_frame_by_id(Value(u"running_0"));
  fighter->attach();
  fighter->set_greyscale(0.3);
  fighter->set_mix_color(u"#364791");
  fighter->set_mix_strength(0.3);
  fighter->set_outline_width(1);
  fighter->set_outline_alpha(1);
  fighter->set_outline_color(u"#131C47");
  return fighter;
}

void LittleFunnyAutoGame::update(double dt) { _director.update(dt); }

void LittleFunnyAutoGame::on_resume() {
  _lr = 0;
  _team1.boss = nullptr;
  _team1.soldiers.clear();
  _team2.boss = nullptr;
  _team2.soldiers.clear();
  world().set_paused(false);
  lfw().change_bg(u"");
  lfw().push_cmd({u"LOCK_CAM", u"0"});
  _director.reset();
}

void LittleFunnyAutoGame::on_pause() {
  world().clear();
  _team1.boss = nullptr;
  _team1.soldiers.clear();
  _team2.boss = nullptr;
  _team2.soldiers.clear();
  lfw().push_cmd({u"LOCK_CAM"});
}

}
}
