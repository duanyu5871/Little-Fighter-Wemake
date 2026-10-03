#include "lfw/buff/buff_magic_flute.h"

#include "lfw/defines/entity_enum.h"
#include "lfw/defines/speed_mode.h"
#include "lfw/defines/state_enum.h"
#include "lfw/entity/calc_v.h"

namespace lfw {
namespace buff {
namespace {

MagicFluteEnv g_env;

constexpr double kFluteInjury = 2.0;
constexpr double kFluteInjuryR = 0.5;
constexpr double kFlute2Injury = 1.0;
constexpr double kFlute2InjuryR = 0.5;
constexpr double kFallInjury = 20.0;
constexpr double kToughness = 0.0;
constexpr double kFluteAcc = 3.0;
constexpr double kFlute2Acc = 1.5;
constexpr double kDecay = 0.25;

Value index_by(const Value& holder, const std::u16string& k) {
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(k);
    if (p != nullptr) return *p;
  }
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    const double index = to_number(Value(k));
    if (!(index >= 0) || index >= static_cast<double>(arr->size())) return Value();
    return arr->at(static_cast<size_t>(index));
  }
  return Value();
}

Value index_0(const Value& holder) {
  const Array* arr = as_array(holder);
  if (arr != nullptr) {
    if (arr->size() == 0) return Value();
    return arr->at(0);
  }
  const Object* o = as_object(holder);
  if (o != nullptr) {
    const Value* p = o->get(u"0");
    if (p != nullptr) return *p;
  }
  return Value();
}

void apply_flute_tick(IBuffEntity* attacker, IBuffEntity* victim, double injury, double injury_r) {
  const Value prev_hp = victim->hp();
  victim->set_hp_r(Value(to_number(victim->hp_r()) - injury_r));
  victim->set_hp(Value(to_number(victim->hp()) - injury));
  victim->set_fallinjury(Value(kFallInjury));
  victim->set_toughness(Value(kToughness));
  if (attacker != nullptr) {
    g_env.summary_apply_damage(attacker, Value(injury), victim, prev_hp);
  }
}

void apply_flute_update(IBuffEntity* attacker, IBuffEntity* victim, double acc) {
  const double vy = entity::calc_v(victim->velocity_y(), acc,
                                   Value(static_cast<double>(SpeedMode::AccTo)), Value(acc),
                                   Value(1.0));
  victim->set_velocity(Value(NullTag{}), Value(vy), Value());
  victim->handle_velocity_decay(kDecay);
  const Value state = victim->state();
  const Value type = victim->data_type();
  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Fighter)))) {
    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Falling)))) {
      victim->enter_frame_by_id(
          to_string(index_0(index_by(victim->data_indexes_falling(), u"-1"))));
    }
    return;
  }
  if (strict_equals(type, Value(static_cast<double>(EntityEnum::Weapon)))) {
    if (!strict_equals(state, Value(static_cast<double>(StateEnum::Weapon_InTheSky))) &&
        !strict_equals(state, Value(static_cast<double>(StateEnum::HeavyWeapon_InTheSky)))) {
      if (attacker != nullptr) victim->set_team(attacker->team());
      victim->enter_frame_by_id(to_string(index_0(victim->data_indexes_in_the_skys())));
    }
  }
}

}

const MagicFluteEnv& magic_flute_env() { return g_env; }

void set_magic_flute_env(const MagicFluteEnv& env) { g_env = env; }

void Buff_MagicFlute::init() {
  set_ticks(3);
  set_duration(3);
}

void Buff_MagicFlute::on_tick(IBuffEntity* attacker, IBuffEntity* victim) {
  if (victim == nullptr) return;
  apply_flute_tick(attacker, victim, kFluteInjury, kFluteInjuryR);
}

void Buff_MagicFlute::on_update(IBuffEntity* attacker, IBuffEntity* victim) {
  if (victim == nullptr) return;
  apply_flute_update(attacker, victim, kFluteAcc);
}

void Buff_MagicFlute2::init() {
  set_ticks(3);
  set_duration(3);
}

void Buff_MagicFlute2::on_tick(IBuffEntity* attacker, IBuffEntity* victim) {
  if (victim == nullptr) return;
  apply_flute_tick(attacker, victim, kFlute2Injury, kFlute2InjuryR);
}

void Buff_MagicFlute2::on_update(IBuffEntity* attacker, IBuffEntity* victim) {
  if (victim == nullptr) return;
  apply_flute_update(attacker, victim, kFlute2Acc);
}

}
}
