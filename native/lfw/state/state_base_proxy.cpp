#include "lfw/state/state_base_proxy.h"

#include <memory>
#include <string>

#include "lfw/defines/weapon_type.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/state/spawn_ice_piece.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace state {
namespace {

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

Value sound_list(const char16_t* path) {
  auto arr = std::make_shared<Array>();
  arr->push_back(Value(std::u16string(path)));
  return Value(arr);
}

void sf_on_landing(IStateEntity& e, const Value& velocity) {
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  const Value vy = field_or(velocity, u"y");
  if (to_number(vy) <= to_number(e.world_dataset(u"cha_bc_tst_spd_y")) * 2) {
    e.enter_frame_by_id(to_string(index_0(index_by(e.data_indexes_bouncing(), u"-1"))));
    e.set_velocity(Value(NullTag{}), e.world_dataset(u"cha_bc_spd"), Value());
    e.set_hp(Value(to_number(e.hp()) - 10));
  }
}

}

StateBase_Proxy::StateBase_Proxy(Value state) : StateBase_Proxy(state, nullptr) {}

StateBase_Proxy::StateBase_Proxy(Value state,
                                 std::unique_ptr<CharacterState_Base> character_proxy)
    : State_Base(state),
      _character_proxy(character_proxy != nullptr
                           ? std::move(character_proxy)
                           : std::make_unique<CharacterState_Base>(state)),
      _weapon_proxy(state),
      _ball_proxy(state),
      _proxy(state) {
  pre_update = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.pre_update) p.pre_update(e);
  };
  enter = [this](IStateEntity& e, const Value& prev_frame) {
    State_Base& p = get_proxy(e);
    if (p.enter) p.enter(e, prev_frame);
  };
  on_dead = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.on_dead) p.on_dead(e);
  };
  on_landing = [this](IStateEntity& e, const Value& velocity) {
    State_Base& p = get_proxy(e);
    if (p.on_landing) p.on_landing(e, velocity);
  };
  get_gravity = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.get_gravity) return p.get_gravity(e);
    return Value();
  };
  get_sudden_death_frame = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.get_sudden_death_frame) return p.get_sudden_death_frame(e);
    return Value();
  };
  get_caught_end_frame = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.get_caught_end_frame) return p.get_caught_end_frame(e);
    return Value();
  };
  get_auto_frame = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.get_auto_frame) return p.get_auto_frame(e);
    return Value();
  };
  find_frame_by_id = [this](IStateEntity& e, const Value& id) {
    State_Base& p = get_proxy(e);
    if (p.find_frame_by_id) return p.find_frame_by_id(e, id);
    return Value();
  };
  on_leave_ground = [this](IStateEntity& e) {
    State_Base& p = get_proxy(e);
    if (p.on_leave_ground) p.on_leave_ground(e);
  };
}

State_Base& StateBase_Proxy::get_proxy(IStateEntity& e) {
  const Value data = e.data();
  if (entity::is_fighter_data(data)) return *_character_proxy;
  if (entity::is_weapon_data(data)) return _weapon_proxy;
  if (entity::is_ball_data(data)) return _ball_proxy;
  return _proxy;
}

void StateBase_Proxy::update(IStateEntity& e) { get_proxy(e).update(e); }

void StateBase_Proxy::leave(IStateEntity& e, const Value& next_frame) {
  get_proxy(e).leave(e, next_frame);
}

void StateBase_Proxy::on_restrict(IStateEntity& e, double x, double y, double z) {
  get_proxy(e).on_restrict(e, x, y, z);
}

State_15::State_15() : StateBase_Proxy(Value(static_cast<double>(StateEnum::Normal))) {}

State_Frozen::State_Frozen(Value state) : StateBase_Proxy(state) {
  const auto super_enter = enter;
  enter = [super_enter](IStateEntity& e, const Value& prev_frame) {
    if (super_enter) super_enter(e, prev_frame);
    if (e.has_catcher()) e.catcher_drop_catching();
    if (equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {
      e.drop_holding();
    }
    e.play_sound(sound_list(u"data/065.wav.mp3"));
  };
  on_landing = &sf_on_landing;
}

void State_Frozen::leave(IStateEntity& e, const Value& next_frame) {
  StateBase_Proxy::leave(e, next_frame);
  e.play_sound(sound_list(u"data/066.wav.mp3"));
  e.apply_opoints(ice_piece_opoints());
  StateBase_Proxy::leave(e, next_frame);
}

}
}
