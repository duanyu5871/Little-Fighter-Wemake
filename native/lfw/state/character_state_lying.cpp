#include "lfw/state/character_state_lying.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "lfw/defines/game_key.h"
#include "lfw/defines/team_enum.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/js_add.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace state {
namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) ||
         std::holds_alternative<NullTag>(v);
}

bool set_has(const std::vector<Value>& xs, const Value& needle) {
  for (size_t i = 0; i < xs.size(); ++i) {
    if (strict_equals(xs[i], needle)) return true;
  }
  return false;
}

void csl_on_dead(IStateEntity& e) {
  std::vector<Value> player_teams;
  const Value puppets = e.world_puppets();
  const Array* arr = as_array(puppets);
  for (size_t i = 0; arr != nullptr && i < arr->size(); ++i) {
    player_teams.push_back(lfw::field_or(arr->at(i), u"team"));
  }
  if (truthy(e.reserve())) e.set_reserve(Value(to_number(e.reserve()) - 1.0));
  if (truthy(e.reserve()) && set_has(player_teams, e.team())) {
    e.blink_and_respawn(e.world_dataset(u"gone_blink_time"));
  } else if (truthy(e.dead_join())) {
    // 死亡后加入
  } else if (truthy(e.dead_gone())) {
    e.blink_and_gone(e.world_dataset(u"gone_blink_time"));
  }
}

}

CharacterState_Lying::CharacterState_Lying(Value state)
    : CharacterState_Base(std::move(state)) {
  enter = [this](IStateEntity& e, const Value& prev_frame) {
    (void)prev_frame;
    e.set_lying_a_count(Value(0.0));
    e.set_lying_d_count(Value(0.0));
    e.set_lying_c_count(Value(0.0));
    e.ctrl_reset_key_list();
    const bool holding = e.has_holding();
    if (holding) e.drop_holding();
    if (holding &&
        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
      e.holding_set_team(e.team());
    }
    e.set_toughness(e.toughness_max());
    e.set_toughness_resting(Value(0.0));
    if (to_number(e.hp()) <= 0 && on_dead) on_dead(e);
  };
  on_dead = &csl_on_dead;
  find_frame_by_id = [](IStateEntity& e, const Value& id) -> Value {
    (void)id;
    double px = 0;
    double py = 0;
    double pz = 0;
    e.position(px, py, pz);
    if (to_number(e.hp()) <= 0 &&
        py <= to_number(e.ground_y()) &&
        strict_equals(e.state(), Value(static_cast<double>(StateEnum::Lying))) &&
        !truthy(e.dead_join())) {
      return e.frame_info();
    }
    return Value();
  };
}

void CharacterState_Lying::update(IStateEntity& e) {
  CharacterState_Base::update(e);
  const Value count_c = e.lying_c_count();
  const Value count_a = e.lying_a_count();
  const bool pressing_a = !e.ctrl_is_end(std::u16string(gk::ka));
  e.set_lying_a_count(js_add(count_a, Value(1.0)));
  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) && pressing_a &&
      to_number(e.wait()) > 0) {
    e.set_lying_c_count(js_add(count_c, Value(1.0)));
    e.set_wait(Value(
        round_float(to_number(e.wait()) - to_number(e.world_dataset(u"atom_time")))));
    return;
  }
  const Value count_d = e.lying_d_count();
  const bool pressing_d = !e.ctrl_is_end(std::u16string(gk::kd));
  e.set_lying_d_count(js_add(count_d, Value(1.0)));
  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 2.0))) && pressing_d) {
    e.set_lying_c_count(js_add(count_c, Value(1.0)));
    e.set_wait(Value(
        round_float(to_number(e.wait()) + to_number(e.world_dataset(u"atom_time")))));
  }
}

void CharacterState_Lying::leave(IStateEntity& e, const Value& next_frame) {
  (void)next_frame;
  if (truthy(e.dead_join()) && to_number(e.hp()) <= 0) {
    const Value dead_join = e.dead_join();
    e.set_motionless(Value(30.0));
    e.set_invulnerable(Value(30.0));
    const Value join_hp = lfw::field_or(dead_join, u"hp");
    const Value next_hp = is_nullish(join_hp) ? e.hp_max() : join_hp;
    e.set_hp_max(next_hp);
    e.set_hp_r(next_hp);
    e.set_hp(next_hp);
    const Value join_team = lfw::field_or(dead_join, u"team");
    e.set_team(is_nullish(join_team) ? Value(std::u16string(team_enum::kTeam_1)) : join_team);
    const Value join_reserve = lfw::field_or(dead_join, u"reserve");
    e.set_reserve(is_nullish(join_reserve) ? Value(0.0) : join_reserve);
    double px = 0;
    double py = 0;
    double pz = 0;
    e.position(px, py, pz);
    e.world_etc(px, py, pz, u"6");
    e.set_outline_color(std::u16string());
    e.set_dead_join(Value(NullTag{}));
    e.set_wakeup_invuln(Value(1.0));
  }
  if (truthy(e.wakeup_invuln())) {
    e.set_blinking(e.world_dataset(u"lying_blink_time"));
    e.set_invulnerable(e.world_dataset(u"lying_blink_time"));
  }
}

}
}
